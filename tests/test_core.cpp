// Unit/integration tests using MockSensorDevice (NOT the kernel driver).
#include <cstdio>
#include <functional>
#include <cstdlib>
#include <thread>
#include "../src/monitor.hpp"

static int fails = 0, total = 0;
#define CHECK(c) do { ++total; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } else std::printf("ok   %s\n", #c); } while (0)

static bool waitFor(std::function<bool()> f, int ms = 3000) {
    for (int i = 0; i < ms / 20; ++i) { if (f()) return true; std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
    return false;
}

int main() {
    // --- ThresholdManager ---
    ThresholdManager t;
    CHECK(t.classify(Status::NORMAL, 39.9) == Status::NORMAL);
    CHECK(t.classify(Status::NORMAL, 40.0) == Status::WARNING);
    CHECK(t.classify(Status::NORMAL, 50.0) == Status::CRITICAL);
    CHECK(t.classify(Status::WARNING, 38.5) == Status::WARNING);   // hysteresis
    CHECK(t.classify(Status::WARNING, 37.9) == Status::NORMAL);
    CHECK(t.classify(Status::CRITICAL, 48.5) == Status::CRITICAL); // hysteresis
    CHECK(t.classify(Status::CRITICAL, 45.0) == Status::WARNING);
    CHECK(ThresholdManager::validate(40, 10, 1000).empty());
    CHECK(!ThresholdManager::validate(-5, 10, 1000).empty());
    CHECK(!ThresholdManager::validate(40, 10, 50).empty());
    CHECK(!ThresholdManager::validate(NAN, 10, 1000).empty());
    CHECK(DataProcessor::valid(25.0));
    CHECK(!DataProcessor::valid(500.0));
    CHECK(!DataProcessor::valid(NAN));

    // --- Monitor with mock device, persistent log ---
    std::remove("/tmp/vs_test.csv");
    auto dev = std::make_unique<MockSensorDevice>();
    MockSensorDevice* mock = dev.get();
    {
        Monitor m(std::move(dev), "/tmp/vs_test.csv", false);
        CHECK(m.setConfig(40, 10, 100).empty());
        CHECK(!m.setConfig(40, 10, 5).empty());
        m.startThread();
        CHECK(waitFor([&] { return m.latestJson().find("\"available\":true") != std::string::npos; }));
        CHECK(m.status() == Status::NORMAL);
        CHECK(m.alertCount() == 0);
        CHECK(m.setSimMode("high").empty());
        CHECK(waitFor([&] { return m.status() == Status::CRITICAL; }));
        size_t a = m.alertCount();
        CHECK(a >= 1 && a <= 2);                       // WARNING then CRITICAL at most
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        CHECK(m.alertCount() == a);                    // no duplicate alerts while staying high
        CHECK(m.setSimMode("low").empty());
        CHECK(waitFor([&] { return m.status() == Status::NORMAL; }));
        CHECK(!m.setSimMode("bogus").empty());
        mock->setBroken(true);                         // missing-device handling
        CHECK(waitFor([&] { return m.statusJson().find("\"device_available\":false") != std::string::npos; }));
        CHECK(waitFor([&] { return m.statusJson().find("\"last_error\":\"\"") == std::string::npos; }));  // monitor loop saw the failure
        mock->setBroken(false);
        m.setMonitoring(false);
        CHECK(m.statusJson().find("\"monitoring\":false") != std::string::npos);
        m.shutdown();
    }
    // restart: log persisted
    {
        EventLogger l("/tmp/vs_test.csv");
        bool sawAlert = false, sawErr = false;
        for (auto& e : l.recent()) { if (e.type == "ALERT") sawAlert = true; if (e.type == "DEVICE_ERROR") sawErr = true; }
        CHECK(l.recent().size() > 5);
        CHECK(sawAlert);
        CHECK(sawErr);
    }
    // log file error handled gracefully
    {
        EventLogger bad("/nonexistent_dir/x.csv");
        bad.log(Event{nowIso(), "s", 1, 2, "NORMAL", "READING", "m"});
        CHECK(!bad.lastError().empty());
    }
    std::printf("\n%d/%d checks passed\n", total - fails, total);
    return fails ? 1 : 0;
}
