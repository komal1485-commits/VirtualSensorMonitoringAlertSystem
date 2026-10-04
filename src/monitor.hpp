// Monitor: owns the device, thresholds, state machine, alerts, log. Source of truth.
#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include "core.hpp"
#include "sensor_device.hpp"

inline std::string jesc(const std::string& s) {
    std::string o;
    for (char c : s) { if (c == '"' || c == '\\') o += '\\'; if (c == '\n') { o += "\\n"; continue; } o += c; }
    return o;
}
inline std::string num(double v) { char b[32]; std::snprintf(b, sizeof b, "%.2f", v); return b; }

class Monitor {
public:
    Monitor(std::unique_ptr<SensorDevice> dev, std::string logPath, bool verbose)
        : dev_(std::move(dev)), logger_(std::move(logPath)), verbose_(verbose) {
        logEvent("MONITOR", "Application started (source " + dev_->source() + ", SIMULATED data)", 0);
        if (!logger_.lastError().empty()) std::fprintf(stderr, "warning: %s\n", logger_.lastError().c_str());
    }
    ~Monitor() { shutdown(); }

    void startThread() { th_ = std::thread([this] { loop(); }); }
    void shutdown() {
        { std::lock_guard<std::mutex> g(m_); stop_ = true; }
        cv_.notify_all();
        if (th_.joinable()) th_.join();
        std::lock_guard<std::mutex> g(m_);
        if (!shutLogged_) { shutLogged_ = true; logEvent("MONITOR", "Application stopped", 0); }
    }

    void setMonitoring(bool on) {
        std::lock_guard<std::mutex> g(m_);
        if (monitoring_ == on) return;
        monitoring_ = on;
        logEvent("MONITOR", on ? "Monitoring started" : "Monitoring stopped", 0);
        cv_.notify_all();
    }
    // Returns "" on success or error message.
    std::string setConfig(double warning, double offset, int intervalMs) {
        std::string err = ThresholdManager::validate(warning, offset, intervalMs);
        if (!err.empty()) return err;
        std::lock_guard<std::mutex> g(m_);
        th_cfg_.warning = warning; th_cfg_.criticalOffset = offset; th_cfg_.intervalMs = intervalMs;
        logEvent("CONFIG", "warning=" + num(warning) + " critical=" + num(th_cfg_.critical()) +
                 " interval_ms=" + std::to_string(intervalMs), warning);
        cv_.notify_all();
        return "";
    }
    std::string setSimMode(const std::string& m) {
        try {
            if (m == "normal") dev_->setMode(VS_MODE_NORMAL);
            else if (m == "high") dev_->setMode(VS_MODE_HIGH);
            else if (m == "low") dev_->setMode(VS_MODE_LOW);
            else if (m == "reset") { dev_->reset(); std::lock_guard<std::mutex> g(m_); status_ = Status::NORMAL; }
            else return "mode must be normal, high, low or reset";
        } catch (const std::exception& e) { return e.what(); }
        std::lock_guard<std::mutex> g(m_);
        simMode_ = m == "reset" ? "normal" : m;
        logEvent("SIM_MODE", "Simulation mode set to " + m + " (simulated)", 0);
        return "";
    }

    // ---- JSON views ----
    std::string statusJson() {
        bool avail = dev_->probe();  // real check, outside the state lock
        std::lock_guard<std::mutex> g(m_);
        std::string j = "{\"simulated\":true,\"source\":\"" + jesc(dev_->source()) + "\"";
        j += ",\"device_available\":" + std::string(avail ? "true" : "false");
        j += ",\"device_checked_at\":\"" + nowIso() + "\"";
        j += ",\"monitoring\":" + std::string(monitoring_ ? "true" : "false");
        j += ",\"last_reading_ts\":\"" + lastTs_ + "\"";
        j += ",\"last_error\":\"" + jesc(lastErr_) + "\"";
        j += ",\"log_error\":\"" + jesc(logger_.lastError()) + "\",\"sim_mode\":\"" + simMode_ + "\"}";
        return j;
    }
    std::string latestJson() {
        std::lock_guard<std::mutex> g(m_);
        if (history_.empty()) return "{\"simulated\":true,\"available\":false}";
        return "{\"simulated\":true,\"available\":true," + readingBody(history_.back()) + "}";
    }
    std::string historyJson(size_t n) {
        std::lock_guard<std::mutex> g(m_);
        std::string j = "{\"simulated\":true,\"readings\":[";
        size_t start = history_.size() > n ? history_.size() - n : 0;
        for (size_t i = start; i < history_.size(); ++i) { if (i > start) j += ","; j += "{" + readingBody(history_[i]) + "}"; }
        return j + "]}";
    }
    std::string alertsJson() {
        std::lock_guard<std::mutex> g(m_);
        return eventArray(alerts_, "alerts", 50);
    }
    std::string eventsJson(const std::string& type, const std::string& status) {
        std::lock_guard<std::mutex> g(m_);
        std::deque<Event> f;
        for (auto& e : logger_.recent())
            if ((type.empty() || e.type == type) && (status.empty() || e.status == status)) f.push_back(e);
        return eventArray(f, "events", 200);
    }
    std::string eventsCsv() { std::lock_guard<std::mutex> g(m_); return logger_.csv(); }
    std::string configJson() {
        std::lock_guard<std::mutex> g(m_);
        return "{\"warning\":" + num(th_cfg_.warning) + ",\"critical\":" + num(th_cfg_.critical()) +
               ",\"critical_offset\":" + num(th_cfg_.criticalOffset) +
               ",\"interval_ms\":" + std::to_string(th_cfg_.intervalMs) + "}";
    }
    // Test hooks
    Status status() { std::lock_guard<std::mutex> g(m_); return status_; }
    size_t alertCount() { std::lock_guard<std::mutex> g(m_); return alerts_.size(); }

private:
    struct Reading { std::string ts; double temp; double threshold; Status status; uint32_t seq; };

    std::string readingBody(const Reading& r) {
        return "\"timestamp\":\"" + r.ts + "\",\"sensor\":\"virtual_temp\",\"temperature_c\":" + num(r.temp) +
               ",\"threshold_c\":" + num(r.threshold) + ",\"status\":\"" + toStr(r.status) + "\",\"seq\":" + std::to_string(r.seq);
    }
    std::string eventArray(const std::deque<Event>& d, const char* key, size_t n) {
        std::string j = std::string("{\"simulated\":true,\"") + key + "\":[";
        size_t start = d.size() > n ? d.size() - n : 0;
        for (size_t i = start; i < d.size(); ++i) {
            const Event& e = d[i];
            if (i > start) j += ",";
            j += "{\"timestamp\":\"" + e.ts + "\",\"sensor\":\"" + e.sensor + "\",\"reading_c\":" + num(e.reading) +
                 ",\"threshold_c\":" + num(e.threshold) + ",\"status\":\"" + e.status + "\",\"event_type\":\"" + e.type +
                 "\",\"message\":\"" + jesc(e.message) + "\"}";
        }
        return j + "]}";
    }
    // Caller holds m_.
    void logEvent(const std::string& type, const std::string& msg, double reading) {
        Event e{nowIso(), "virtual_temp", reading, th_cfg_.warning, toStr(status_), type, msg};
        logger_.log(e);
        if (type == "ALERT") { alerts_.push_back(e); if (alerts_.size() > 200) alerts_.pop_front(); }
        if (verbose_) std::printf("[%s] %-12s %s\n", e.ts.c_str(), type.c_str(), msg.c_str());
    }

    void tick() {
        Sample s;
        try { s = dev_->read(); }
        catch (const std::exception& ex) {
            std::lock_guard<std::mutex> g(m_);
            if (lastErr_.empty()) logEvent("DEVICE_ERROR", ex.what(), 0);  // log once per outage
            lastErr_ = ex.what();
            return;
        }
        std::lock_guard<std::mutex> g(m_);
        if (!lastErr_.empty()) { lastErr_.clear(); logEvent("MONITOR", "Device recovered", 0); }
        if (!DataProcessor::valid(s.temp_c)) { logEvent("INVALID_READING", "Reading out of range rejected", s.temp_c); return; }
        Status prev = status_;
        status_ = th_cfg_.classify(prev, s.temp_c);
        lastTs_ = nowIso();
        history_.push_back({lastTs_, s.temp_c, th_cfg_.warning, status_, s.seq});
        if (history_.size() > 600) history_.pop_front();
        logEvent("READING", "simulated sample seq=" + std::to_string(s.seq), s.temp_c);
        if (status_ != prev) {
            if (status_ > prev)
                logEvent("ALERT", std::string(toStr(status_)) + ": " + num(s.temp_c) + " C reached limit " +
                         num(status_ == Status::CRITICAL ? th_cfg_.critical() : th_cfg_.warning) + " C", s.temp_c);
            else
                logEvent("STATE_CHANGE", std::string("Recovered to ") + toStr(status_), s.temp_c);
        }
    }

    void loop() {
        std::unique_lock<std::mutex> lk(m_);
        while (!stop_) {
            if (monitoring_) { lk.unlock(); tick(); lk.lock(); }
            cv_.wait_for(lk, std::chrono::milliseconds(th_cfg_.intervalMs), [this] { return stop_; });
        }
    }

    std::unique_ptr<SensorDevice> dev_;
    EventLogger logger_;
    ThresholdManager th_cfg_;
    bool verbose_;
    std::mutex m_;
    std::condition_variable cv_;
    std::thread th_;
    bool stop_ = false, monitoring_ = true, shutLogged_ = false;
    Status status_ = Status::NORMAL;
    std::string lastTs_, lastErr_, simMode_ = "normal";
    std::deque<Reading> history_;
    std::deque<Event> alerts_;
};
