#include "config/database.hpp"
#include "controllers/auth_controller.hpp"
#include "utils/http_utils.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

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

    try {
        config::ensure_extensions();
    } catch (const std::exception& err) {
        std::cerr << "Database initialization error: " << err.what() << '\n';
        return 1;
    }

    httplib::Server server;

    server.Get("/", [](const httplib::Request&, httplib::Response& res) {
        utils::send_json(res, 200, {{"message", "API funcionando"}});
    });

    server.Post("/api/auth/register", controllers::register_user);
    server.Post("/api/auth/login", controllers::login_user);
    server.Get("/api/auth/me", controllers::me);

    server.Options("/", controllers::options_ok);
    server.Options("/api/auth/register", controllers::options_ok);
    server.Options("/api/auth/login", controllers::options_ok);
    server.Options("/api/auth/me", controllers::options_ok);

    std::cout << "Servidor corriendo en puerto " << port << '\n';

    if (!server.listen(host, port)) {
        std::cerr << "Failed to start server on " << host << ':' << port << '\n';
        return 1;
    }

    return 0;
}
