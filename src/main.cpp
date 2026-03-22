#include <cstdlib>
#include <iostream>
#include <string>

#include <httplib.h>

namespace {

std::string get_env_or_default(const char* key, const char* fallback) {
    if (const char* value = std::getenv(key)) {
        return value;
    }
    return fallback;
}

int get_port() {
    const std::string port = get_env_or_default("PORT", "8080");

    try {
        return std::stoi(port);
    } catch (...) {
        return 8080;
    }
}

}  // namespace

int main() {
    const std::string host = get_env_or_default("HOST", "0.0.0.0");
    const int port = get_port();

    httplib::Server server;

    server.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"service":"backend","status":"ok"})", "application/json");
    });

    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"healthy"})", "application/json");
    });

    std::cout << "Listening on " << host << ':' << port << '\n';

    if (!server.listen(host, port)) {
        std::cerr << "Failed to start server on " << host << ':' << port << '\n';
        return 1;
    }

    return 0;
}
