#include <csignal>
#include <iostream>
#include "dashboard_server.hpp"

static std::atomic<bool> g_stop{false};
static void onSignal(int) { g_stop.store(true); }

int main(int argc, char** argv) {
    std::string device = VS_DEVICE_PATH, logPath = "logs/events.csv", web = "web";
    int port = 8080;
    bool mock = false, verbose = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { if (i + 1 >= argc) { std::cerr << "missing value for " << a << "\n"; std::exit(2); } return argv[++i]; };
        if (a == "--device") device = next();
        else if (a == "--log") logPath = next();
        else if (a == "--web") web = next();
        else if (a == "--port") port = std::atoi(next().c_str());
        else if (a == "--mock") mock = true;
        else if (a == "--verbose") verbose = true;
        else { std::cerr << "usage: monitor [--device PATH] [--mock] [--port N] [--log FILE] [--web DIR] [--verbose]\n"; return 2; }
    }
    if (port < 1024 || port > 65535) { std::cerr << "port must be 1024-65535\n"; return 2; }

    struct sigaction sa{};
    sa.sa_handler = onSignal;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    std::unique_ptr<SensorDevice> dev;
    if (mock) { dev = std::make_unique<MockSensorDevice>(); std::cerr << "NOTE: --mock: user-space simulator, NOT the kernel driver\n"; }
    else dev = std::make_unique<KernelSensorDevice>(device);

    try {
        Monitor mon(std::move(dev), logPath, verbose);
        mon.startThread();
        DashboardServer srv(mon, port, web);
        std::cerr << "Dashboard: http://127.0.0.1:" << port << "/  (Ctrl-C to stop)\n";
        srv.run(g_stop);
        mon.shutdown();
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
    std::cerr << "shut down cleanly\n";
    return 0;
}
