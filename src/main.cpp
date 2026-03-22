#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "config/database.hpp"
#include "controllers/auth_controller.hpp"
#include "utils/http_utils.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <cctype>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
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

int get_https_port() {
    const std::string port = get_env_or_default("HTTPS_PORT", "8443");

    try {
        return std::stoi(port);
    } catch (...) {
        return 8443;
    }
}

bool env_truthy(const char* key) {
    const std::string value = get_env_or_default(key, "");
    if (value.empty()) return false;

    const std::string lower = [&value]() {
        std::string s = value;
        for (char& c : s) c = static_cast<char>(std::tolower(c));
        return s;
    }();

    return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

}  // namespace

int main() {
    const std::string host = get_env_or_default("HOST", "0.0.0.0");
    const bool https_enabled = env_truthy("HTTPS_ENABLED");
    const int port = https_enabled ? get_https_port() : get_port();

    // Si se habilita HTTPS y no se pasan rutas, usamos cert.pem/key.pem en el cwd.
    std::string cert_file = get_env_or_default("HTTPS_CERT_FILE", https_enabled ? "cert.pem" : "");
    std::string key_file = get_env_or_default("HTTPS_KEY_FILE", https_enabled ? "key.pem" : "");

    try {
        config::ensure_extensions();
    } catch (const std::exception& err) {
        std::cerr << "Database initialization error: " << err.what() << '\n';
        return 1;
    }

    std::unique_ptr<httplib::Server> server;

    if (https_enabled) {
        if (cert_file.empty() || key_file.empty()) {
            std::cerr << "HTTPS_ENABLED=1 pero faltan HTTPS_CERT_FILE o HTTPS_KEY_FILE\n";
            return 1;
        }

        const std::filesystem::path cert_path = std::filesystem::absolute(cert_file);
        const std::filesystem::path key_path = std::filesystem::absolute(key_file);

        if (!std::filesystem::exists(cert_path)) {
            std::cerr << "Certificado no encontrado en " << cert_path.string() << '\n';
            return 1;
        }
        if (!std::filesystem::exists(key_path)) {
            std::cerr << "Clave privada no encontrada en " << key_path.string() << '\n';
            return 1;
        }

        server = std::make_unique<httplib::SSLServer>(cert_path.string().c_str(), key_path.string().c_str());

        if (auto* ssl = dynamic_cast<httplib::SSLServer*>(server.get())) {
            if (!ssl->is_valid()) {
                std::cerr << "No se pudo inicializar el servidor HTTPS. Revisa cert/key.\n";
                return 1;
            }
        }
    } else {
        server = std::make_unique<httplib::Server>();
    }

    server->Get("/", [](const httplib::Request&, httplib::Response& res) {
        utils::send_json(res, 200, {{"message", "API funcionando"}});
    });

    server->Post("/api/auth/register", controllers::register_user);
    server->Post("/api/auth/login", controllers::login_user);
    server->Get("/api/auth/me", controllers::me);

    server->Options("/", controllers::options_ok);
    server->Options("/api/auth/register", controllers::options_ok);
    server->Options("/api/auth/login", controllers::options_ok);
    server->Options("/api/auth/me", controllers::options_ok);

    std::cout << (https_enabled ? "Servidor HTTPS" : "Servidor HTTP") << " corriendo en puerto " << port << '\n';

    if (!server->listen(host, port)) {
        std::cerr << "Failed to start server on " << host << ':' << port << '\n';
        return 1;
    }

    return 0;
}
