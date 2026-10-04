// SensorDevice: access to the (simulated) sensor. KernelSensorDevice talks to
// /dev/virtual_sensor via open/read/ioctl; MockSensorDevice is a user-space
// stand-in used ONLY for development/testing without the kernel module.
#pragma once
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../include/vs_ioctl.h"

struct Sample {
    int64_t ts_ns = 0;
    double temp_c = 0.0;
    uint32_t seq = 0;
    uint32_t mode = 0;
};

class SensorDevice {
public:
    virtual ~SensorDevice() = default;
    virtual bool probe() = 0;                 // actually checks the device now
    virtual Sample read() = 0;                // throws std::runtime_error
    virtual void setMode(uint32_t mode) = 0;  // throws std::runtime_error
    virtual void reset() = 0;
    virtual std::string source() const = 0;   // "kernel:/dev/..." or "mock"
};

class KernelSensorDevice : public SensorDevice {
public:
    explicit KernelSensorDevice(std::string path) : path_(std::move(path)) {}
    ~KernelSensorDevice() override { if (fd_ >= 0) ::close(fd_); }

    bool probe() override {
        std::lock_guard<std::mutex> g(m_);
        try { ensureOpen(); return true; } catch (...) { return false; }
    }
    Sample read() override {
        std::lock_guard<std::mutex> g(m_);
        ensureOpen();
        vs_sample raw{};
        ssize_t n;
        do { n = ::read(fd_, &raw, sizeof raw); } while (n < 0 && errno == EINTR);
        if (n != (ssize_t)sizeof raw) {
            std::string why = n < 0 ? std::strerror(errno) : "short read";
            closeFd();  // force re-open next time (e.g. module reloaded)
            throw std::runtime_error("read() failed: " + why);
        }
        return Sample{raw.timestamp_ns, raw.temp_mdeg / 1000.0, raw.seq, raw.mode};
    }
    void setMode(uint32_t mode) override {
        std::lock_guard<std::mutex> g(m_);
        ensureOpen();
        if (::ioctl(fd_, VS_IOC_SET_MODE, &mode) < 0)
            throw std::runtime_error(std::string("ioctl(SET_MODE) failed: ") + std::strerror(errno));
    }
    void reset() override {
        std::lock_guard<std::mutex> g(m_);
        ensureOpen();
        if (::ioctl(fd_, VS_IOC_RESET) < 0)
            throw std::runtime_error(std::string("ioctl(RESET) failed: ") + std::strerror(errno));
    }
    std::string source() const override { return "kernel:" + path_; }

private:
    void ensureOpen() {
        if (fd_ >= 0) return;
        fd_ = ::open(path_.c_str(), O_RDWR | O_CLOEXEC);
        if (fd_ < 0)
            throw std::runtime_error("open(" + path_ + ") failed: " + std::strerror(errno));
    }
    void closeFd() { if (fd_ >= 0) { ::close(fd_); fd_ = -1; } }
    std::string path_;
    int fd_ = -1;
    std::mutex m_;
};

// Same model as the kernel driver, implemented in user space. NOT the driver.
class MockSensorDevice : public SensorDevice {
public:
    bool probe() override { return !broken_; }
    Sample read() override {
        std::lock_guard<std::mutex> g(m_);
        if (broken_) throw std::runtime_error("mock device unavailable");
        double target = mode_ == VS_MODE_HIGH ? 60.0 : mode_ == VS_MODE_LOW ? 5.0 : 25.0;
        std::uniform_real_distribution<double> noise(-0.4, 0.4);
        temp_ += (target - temp_) / 5.0 + noise(rng_);
        ++seq_;
        int64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return Sample{now, temp_, seq_, mode_};
    }
    void setMode(uint32_t mode) override {
        std::lock_guard<std::mutex> g(m_);
        if (mode > VS_MODE_LOW) throw std::runtime_error("invalid mode");
        mode_ = mode;
    }
    void reset() override { std::lock_guard<std::mutex> g(m_); mode_ = 0; temp_ = 25.0; seq_ = 0; }
    std::string source() const override { return "mock"; }
    void setBroken(bool b) { std::lock_guard<std::mutex> g(m_); broken_ = b; }  // test hook
private:
    std::mutex m_;
    std::mt19937 rng_{12345};
    double temp_ = 25.0;
    uint32_t seq_ = 0, mode_ = 0;
    bool broken_ = false;
};
