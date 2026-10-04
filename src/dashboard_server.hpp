// Minimal blocking HTTP/1.1 server bound to 127.0.0.1 only.
#pragma once
#include <arpa/inet.h>
#include <atomic>
#include <fstream>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include "monitor.hpp"

class DashboardServer {
public:
    DashboardServer(Monitor& mon, int port, std::string webDir) : mon_(mon), port_(port), web_(std::move(webDir)) {}

    void run(const std::atomic<bool>& stop) {
        int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0) throw std::runtime_error("socket() failed");
        int one = 1;
        ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_port = htons((uint16_t)port_);
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // localhost only
        if (::bind(fd, (sockaddr*)&a, sizeof a) < 0 || ::listen(fd, 16) < 0) {
            std::string e = std::strerror(errno);
            ::close(fd);
            throw std::runtime_error("cannot listen on 127.0.0.1:" + std::to_string(port_) + ": " + e);
        }
        while (!stop.load()) {
            pollfd p{fd, POLLIN, 0};
            if (::poll(&p, 1, 200) <= 0) continue;
            int c = ::accept4(fd, nullptr, nullptr, SOCK_CLOEXEC);
            if (c < 0) continue;
            timeval tv{2, 0};
            ::setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
            try { handle(c); } catch (...) {}
            ::close(c);
        }
        ::close(fd);
    }

private:
    static std::string jsonStr(const std::string& b, const std::string& key) {
        size_t k = b.find("\"" + key + "\""); if (k == std::string::npos) return "";
        size_t c = b.find(':', k), q1 = b.find('"', c), q2 = b.find('"', q1 + 1);
        return (q1 == std::string::npos || q2 == std::string::npos) ? "" : b.substr(q1 + 1, q2 - q1 - 1);
    }
    static bool jsonNum(const std::string& b, const std::string& key, double& out) {
        size_t k = b.find("\"" + key + "\""); if (k == std::string::npos) return false;
        size_t c = b.find(':', k); if (c == std::string::npos) return false;
        try { size_t used = 0; out = std::stod(b.substr(c + 1), &used); return used > 0; } catch (...) { return false; }
    }
    static std::string query(const std::string& q, const std::string& key) {
        size_t k = q.find(key + "="); if (k == std::string::npos) return "";
        size_t e = q.find('&', k);
        return q.substr(k + key.size() + 1, e == std::string::npos ? std::string::npos : e - k - key.size() - 1);
    }
    static void sendAll(int c, const std::string& s) {
        size_t off = 0;
        while (off < s.size()) { ssize_t n = ::send(c, s.data() + off, s.size() - off, MSG_NOSIGNAL); if (n <= 0) return; off += n; }
    }
    static void reply(int c, int code, const std::string& type, const std::string& body, const std::string& extra = "") {
        const char* txt = code == 200 ? "OK" : code == 400 ? "Bad Request" : code == 404 ? "Not Found" : "Error";
        sendAll(c, "HTTP/1.1 " + std::to_string(code) + " " + txt + "\r\nContent-Type: " + type +
                   "\r\nContent-Length: " + std::to_string(body.size()) + "\r\nCache-Control: no-store\r\n" + extra +
                   "Connection: close\r\n\r\n" + body);
    }
    static void json(int c, int code, const std::string& b) { reply(c, code, "application/json", b); }
    static void err(int c, const std::string& m) { json(c, 400, "{\"error\":\"" + jesc(m) + "\"}"); }

    void handle(int c) {
        std::string req;
        char buf[2048];
        size_t hdrEnd = std::string::npos;
        while (req.size() < 16384) {
            ssize_t n = ::recv(c, buf, sizeof buf, 0);
            if (n <= 0) break;
            req.append(buf, n);
            if ((hdrEnd = req.find("\r\n\r\n")) != std::string::npos) break;
        }
        if (hdrEnd == std::string::npos) return;
        size_t clen = 0, cl = req.find("Content-Length:");
        if (cl != std::string::npos) clen = std::min<size_t>(std::strtoul(req.c_str() + cl + 15, nullptr, 10), 4096);
        while (req.size() < hdrEnd + 4 + clen) {
            ssize_t n = ::recv(c, buf, sizeof buf, 0);
            if (n <= 0) break;
            req.append(buf, n);
        }
        std::string body = req.substr(hdrEnd + 4);
        std::istringstream ls(req);
        std::string method, target, ver;
        ls >> method >> target >> ver;
        std::string path = target, q;
        size_t qm = target.find('?');
        if (qm != std::string::npos) { path = target.substr(0, qm); q = target.substr(qm + 1); }

        if (method == "GET") {
            if (path == "/" || path == "/index.html") return serveIndex(c);
            if (path == "/api/status") return json(c, 200, mon_.statusJson());
            if (path == "/api/readings/latest") return json(c, 200, mon_.latestJson());
            if (path == "/api/readings/history") {
                size_t n = 120; try { std::string v = query(q, "limit"); if (!v.empty()) n = std::min<unsigned long>(std::stoul(v), 600); } catch (...) {}
                return json(c, 200, mon_.historyJson(n));
            }
            if (path == "/api/alerts") return json(c, 200, mon_.alertsJson());
            if (path == "/api/config") return json(c, 200, mon_.configJson());
            if (path == "/api/events") return json(c, 200, mon_.eventsJson(query(q, "type"), query(q, "status")));
            if (path == "/api/events/export")
                return reply(c, 200, "text/csv", mon_.eventsCsv(), "Content-Disposition: attachment; filename=\"events.csv\"\r\n");
            return json(c, 404, "{\"error\":\"not found\"}");
        }
        if (method == "POST") {
            if (path == "/api/config") {
                double w, o = 10, i;
                if (!jsonNum(body, "warning", w) || !jsonNum(body, "interval_ms", i)) return err(c, "need numeric warning and interval_ms");
                jsonNum(body, "critical_offset", o);
                std::string e = mon_.setConfig(w, o, (int)i);
                return e.empty() ? json(c, 200, mon_.configJson()) : err(c, e);
            }
            if (path == "/api/simulation/mode") {
                std::string e = mon_.setSimMode(jsonStr(body, "mode"));
                return e.empty() ? json(c, 200, "{\"ok\":true}") : err(c, e);
            }
            if (path == "/api/monitoring/start") { mon_.setMonitoring(true); return json(c, 200, "{\"ok\":true}"); }
            if (path == "/api/monitoring/stop") { mon_.setMonitoring(false); return json(c, 200, "{\"ok\":true}"); }
            return json(c, 404, "{\"error\":\"not found\"}");
        }
        json(c, 400, "{\"error\":\"unsupported method\"}");
    }

    void serveIndex(int c) {
        std::ifstream f(web_ + "/index.html", std::ios::binary);
        if (!f) return reply(c, 404, "text/plain", "index.html not found in " + web_);
        std::ostringstream o; o << f.rdbuf();
        reply(c, 200, "text/html; charset=utf-8", o.str());
    }

    Monitor& mon_;
    int port_;
    std::string web_;
};
