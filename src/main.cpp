#include "config/app_config.hpp"
#include "config/database.hpp"
#include "config/environment.hpp"
#include "controllers/auth_controller.hpp"
#include "repositories/pqxx_user_repository.hpp"
#include "services/auth_service.hpp"
#include "services/jwt_token_service.hpp"
#include "utils/http_utils.hpp"

#include <httplib.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

int main() {
    try {
        const config::ProcessEnvironment environment;
        const auto app_config = config::AppConfig::from_environment(environment);
        const config::Database database{app_config.database_url};
        database.ensure_extensions();

        repositories::PqxxUserRepository user_repository{database};
        services::JwtTokenService token_service{app_config.jwt_secret};
        services::AuthService auth_service{user_repository, token_service};
        controllers::AuthController auth_controller{auth_service, token_service};

        std::unique_ptr<httplib::Server> server;
        if (app_config.https_enabled) {
            if (app_config.https_cert_file.empty() || app_config.https_key_file.empty()) {
                std::cerr << "HTTPS_ENABLED=1 pero faltan HTTPS_CERT_FILE o HTTPS_KEY_FILE\n";
                return 1;
            }

            const auto cert_path = std::filesystem::absolute(app_config.https_cert_file);
            const auto key_path = std::filesystem::absolute(app_config.https_key_file);

            if (!std::filesystem::exists(cert_path)) {
                std::cerr << "Certificado no encontrado en " << cert_path.string() << '\n';
                return 1;
            }
            if (!std::filesystem::exists(key_path)) {
                std::cerr << "Clave privada no encontrada en " << key_path.string() << '\n';
                return 1;
            }

            server = std::make_unique<httplib::SSLServer>(cert_path.string().c_str(), key_path.string().c_str());
            if (auto* ssl_server = dynamic_cast<httplib::SSLServer*>(server.get()); ssl_server != nullptr && !ssl_server->is_valid()) {
                std::cerr << "No se pudo inicializar el servidor HTTPS. Revisa cert/key.\n";
                return 1;
            }
        } else {
            server = std::make_unique<httplib::Server>();
        }

        server->Get("/", [](const httplib::Request& request, httplib::Response& response) {
            utils::send_json(request, response, 200, {{"message", "API funcionando"}});
        });

        server->Post("/api/auth/register", [&auth_controller](const httplib::Request& request, httplib::Response& response) {
            auth_controller.register_user(request, response);
        });
        server->Post("/api/auth/login", [&auth_controller](const httplib::Request& request, httplib::Response& response) {
            auth_controller.login_user(request, response);
        });
        server->Get("/api/auth/me", [&auth_controller](const httplib::Request& request, httplib::Response& response) {
            auth_controller.me(request, response);
        });

        server->Options("/", [&auth_controller](const httplib::Request& request, httplib::Response& response) {
            auth_controller.options_ok(request, response);
        });
        server->Options("/api/auth/register", [&auth_controller](const httplib::Request& request, httplib::Response& response) {
            auth_controller.options_ok(request, response);
        });
        server->Options("/api/auth/login", [&auth_controller](const httplib::Request& request, httplib::Response& response) {
            auth_controller.options_ok(request, response);
        });
        server->Options("/api/auth/me", [&auth_controller](const httplib::Request& request, httplib::Response& response) {
            auth_controller.options_ok(request, response);
        });

        std::cout << (app_config.https_enabled ? "Servidor HTTPS" : "Servidor HTTP")
                  << " corriendo en puerto " << app_config.active_port() << '\n';

        if (!server->listen(app_config.host, app_config.active_port())) {
            std::cerr << "Failed to start server on " << app_config.host << ':' << app_config.active_port() << '\n';
            return 1;
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Startup error: " << error.what() << '\n';
        return 1;
    }
}
