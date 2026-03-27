#include "controllers/auth_controller.hpp"

#include "utils/http_utils.hpp"

#include <exception>
#include <stdexcept>
#include <string>

namespace controllers {

AuthController::AuthController(services::AuthService& auth_service, const services::TokenService& token_service)
    : auth_service_(auth_service),
      token_service_(token_service) {}

void AuthController::register_user(const httplib::Request& request, httplib::Response& response) const {
    try {
        const auto body = parse_body(request);
        const auto email = read_required_string(body, "email");
        const auto password = read_required_string(body, "password");
        const auto user = auth_service_.register_user(email, password);

        utils::send_json(request, response, 201, {
            {"success", true},
            {"message", "Usuario registrado correctamente"},
            {"data", {{"id", user.id}, {"email", user.email}}}
        });
    } catch (const std::exception& error) {
        utils::send_json(request, response, 400, {
            {"success", false},
            {"message", "Error al registrar el usuario"},
            {"error", error.what()}
        });
    }
}

void AuthController::login_user(const httplib::Request& request, httplib::Response& response) const {
    try {
        const auto body = parse_body(request);
        const auto email = read_required_string(body, "email");
        const auto password = read_required_string(body, "password");
        const auto result = auth_service_.login_user(email, password);

        utils::send_json(request, response, 200, {
            {"success", true},
            {"message", "Login correcto"},
            {"token", result.token}
        });
    } catch (const std::exception& error) {
        utils::send_json(request, response, 401, {
            {"success", false},
            {"message", "Error en el login"},
            {"error", error.what()}
        });
    }
}

void AuthController::me(const httplib::Request& request, httplib::Response& response) const {
    const auto token = extract_bearer_token(request);
    if (token.empty()) {
        utils::send_json(request, response, 401, {{"error", "No autorizado"}});
        return;
    }

    try {
        auto payload = token_service_.verify_token(token);
        nlohmann::json user_payload = {
            {"id", payload.at("id")},
            {"email", payload.at("email")}
        };

        if (payload.contains("iat")) {
            user_payload["iat"] = payload.at("iat");
        }
        if (payload.contains("exp")) {
            user_payload["exp"] = payload.at("exp");
        }

        utils::send_json(request, response, 200, {
            {"success", true},
            {"user", user_payload}
        });
    } catch (...) {
        utils::send_json(request, response, 401, {{"error", "Token invalido"}});
    }
}

void AuthController::options_ok(const httplib::Request& request, httplib::Response& response) const {
    utils::set_cors_headers(request, response);
    response.status = 204;
}

nlohmann::json AuthController::parse_body(const httplib::Request& request) {
    if (request.body.empty()) {
        throw std::runtime_error("Body vacio");
    }

    return nlohmann::json::parse(request.body);
}

std::string AuthController::read_required_string(const nlohmann::json& body, const char* key) {
    if (!body.contains(key) || !body.at(key).is_string()) {
        throw std::runtime_error(std::string{"Campo invalido: "} + key);
    }

    return body.at(key).get<std::string>();
}

std::string AuthController::extract_bearer_token(const httplib::Request& request) {
    const auto header = request.get_header_value("Authorization");
    if (header.empty()) {
        return {};
    }

    constexpr std::string_view prefix{"Bearer "};
    if (header.rfind(prefix.data(), 0) == 0) {
        return header.substr(prefix.size());
    }

    const auto position = header.find(' ');
    if (position == std::string::npos || position + 1 >= header.size()) {
        return {};
    }

    return header.substr(position + 1);
}

}  // namespace controllers
