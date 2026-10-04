// ThresholdManager, DataProcessor, AlertManager, EventLogger (pure logic + CSV I/O).
#pragma once
#include <chrono>
#include <vector>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <deque>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>

enum class Status { NORMAL = 0, WARNING = 1, CRITICAL = 2 };
inline const char* toStr(Status s) {
    return s == Status::NORMAL ? "NORMAL" : s == Status::WARNING ? "WARNING" : "CRITICAL";
}

inline std::string nowIso() {
    using namespace std::chrono;
    auto tp = system_clock::now();
    std::time_t t = system_clock::to_time_t(tp);
    int ms = (int)(duration_cast<milliseconds>(tp.time_since_epoch()).count() % 1000);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[40], out[48];
    std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S", &tm);
    std::snprintf(out, sizeof out, "%s.%03d", buf, ms);
    return out;
}

// Warning at `warning`, critical at warning + criticalOffset.
// Hysteresis prevents flapping (and duplicate alerts) around a threshold.
class ThresholdManager {
public:
    static constexpr double MIN_WARNING = 0, MAX_WARNING = 100;
    static constexpr double MIN_OFFSET = 1, MAX_OFFSET = 30;
    static constexpr int MIN_INTERVAL_MS = 100, MAX_INTERVAL_MS = 10000;

    double warning = 40.0, criticalOffset = 10.0, hysteresis = 2.0;
    int intervalMs = 1000;

    double critical() const { return warning + criticalOffset; }

    Status classify(Status prev, double t) const {
        Status s = Status::NORMAL;
        if (t >= critical()) s = Status::CRITICAL;
        else if (t >= warning) s = Status::WARNING;
        if (prev == Status::CRITICAL && s != Status::CRITICAL && t >= critical() - hysteresis)
            s = Status::CRITICAL;
        if (prev != Status::NORMAL && s == Status::NORMAL && t >= warning - hysteresis)
            s = Status::WARNING;
        return s;
    }
    // Returns empty string if valid, else an error message.
    static std::string validate(double warning, double offset, int intervalMs) {
        if (!std::isfinite(warning) || warning < MIN_WARNING || warning > MAX_WARNING)
            return "threshold must be between 0 and 100 C";
        if (!std::isfinite(offset) || offset < MIN_OFFSET || offset > MAX_OFFSET)
            return "critical offset must be between 1 and 30 C";
        if (intervalMs < MIN_INTERVAL_MS || intervalMs > MAX_INTERVAL_MS)
            return "interval must be between 100 and 10000 ms";
        return "";
    }
};

class DataProcessor {
public:
    static bool valid(double t) { return std::isfinite(t) && t >= -40.0 && t <= 125.0; }
};

struct Event {
    std::string ts, sensor;
    double reading = 0, threshold = 0;
    std::string status, type, message;
};

class EventLogger {
public:
    explicit EventLogger(std::string path) : path_(std::move(path)) { load(); openForAppend(); }

    void log(const Event& e) {
        recent_.push_back(e);
        if (recent_.size() > 1000) recent_.pop_front();
        if (!out_.is_open()) { openForAppend(); if (!out_.is_open()) return; }
        out_ << e.ts << ',' << e.sensor << ',' << fmt(e.reading) << ',' << fmt(e.threshold) << ','
             << e.status << ',' << e.type << ",\"" << esc(e.message) << "\"\n";
        out_.flush();
        if (!out_) { lastError_ = "write to " + path_ + " failed"; out_.close(); }
    }
    const std::deque<Event>& recent() const { return recent_; }
    const std::string& lastError() const { return lastError_; }
    const std::string& path() const { return path_; }

    std::string csv() const {
        std::ostringstream o;
        o << header() << "\n";
        for (auto& e : recent_)
            o << e.ts << ',' << e.sensor << ',' << fmt(e.reading) << ',' << fmt(e.threshold) << ','
              << e.status << ',' << e.type << ",\"" << esc(e.message) << "\"\n";
        return o.str();
    }
    static std::string header() { return "timestamp,sensor,reading_c,threshold_c,status,event_type,message"; }

private:
    static std::string fmt(double v) { char b[32]; std::snprintf(b, sizeof b, "%.2f", v); return b; }
    static std::string esc(std::string m) {
        for (auto& c : m) if (c == '"') c = '\'';
        return m;
    }
    void openForAppend() {
        struct stat st{};
        bool fresh = ::stat(path_.c_str(), &st) != 0 || st.st_size == 0;
        out_.open(path_, std::ios::app);
        if (!out_) { lastError_ = "cannot open log file " + path_; return; }
        if (fresh) out_ << header() << "\n";
    }
    void load() {  // restore recent history after restart
        std::ifstream in(path_);
        std::string line;
        std::getline(in, line);  // header
        while (std::getline(in, line)) {
            std::vector<std::string> f;
            size_t pos = 0;
            for (int i = 0; i < 6; ++i) {
                size_t c = line.find(',', pos);
                if (c == std::string::npos) break;
                f.push_back(line.substr(pos, c - pos));
                pos = c + 1;
            }
            if (f.size() != 6) continue;
            std::string msg = line.substr(pos);
            if (msg.size() >= 2 && msg.front() == '"' && msg.back() == '"') msg = msg.substr(1, msg.size() - 2);
            try {
                Event e{f[0], f[1], std::stod(f[2]), std::stod(f[3]), f[4], f[5], msg};
                recent_.push_back(e);
                if (recent_.size() > 1000) recent_.pop_front();
            } catch (...) { /* skip corrupt line */ }
        }
    }
    std::string path_, lastError_;
    std::ofstream out_;
    std::deque<Event> recent_;
};
