#include "controllers/auth_controller.hpp"

#include "services/auth_service.hpp"
#include "utils/http_utils.hpp"
#include "utils/jwt_utils.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <stdexcept>
#include <string>

namespace controllers {
namespace {

nlohmann::json parse_body(const httplib::Request& req) {
    if (req.body.empty()) {
        throw std::runtime_error("Body vacio");
    }

    return nlohmann::json::parse(req.body);
}

std::string read_required_string(const nlohmann::json& body, const char* key) {
    if (!body.contains(key) || !body.at(key).is_string()) {
        throw std::runtime_error(std::string("Campo invalido: ") + key);
    }

    return body.at(key).get<std::string>();
}

}  // namespace

void register_user(const httplib::Request& req, httplib::Response& res) {
    try {
        const nlohmann::json body = parse_body(req);
        const std::string email = read_required_string(body, "email");
        const std::string password = read_required_string(body, "password");

        const auto user = services::register_user(email, password);

        utils::send_json(res, 201, {
            {"success", true},
            {"message", "Usuario registrado correctamente"},
            {"data", {
                {"id", user.id},
                {"email", user.email}
            }}
        });
    } catch (const std::exception& err) {
        utils::send_json(res, 400, {
            {"success", false},
            {"message", "Error al registrar el usuario"},
            {"error", err.what()}
        });
    }
}

void login_user(const httplib::Request& req, httplib::Response& res) {
    try {
        const nlohmann::json body = parse_body(req);
        const std::string email = read_required_string(body, "email");
        const std::string password = read_required_string(body, "password");

        const auto data = services::login_user(email, password);

        utils::send_json(res, 200, {
            {"success", true},
            {"message", "Login correcto"},
            {"token", data.token}
        });
    } catch (const std::exception& err) {
        utils::send_json(res, 401, {
            {"success", false},
            {"message", "Error en el login"},
            {"error", err.what()}
        });
    }
}

void me(const httplib::Request& req, httplib::Response& res) {
    nlohmann::json user_payload;
    if (!utils::authorize_request(req, res, user_payload)) {
        return;
    }

    utils::send_json(res, 200, {
        {"success", true},
        {"user", user_payload}
    });
}

void options_ok(const httplib::Request&, httplib::Response& res) {
    utils::set_cors_headers(res);
    res.status = 204;
}

}  // namespace controllers
