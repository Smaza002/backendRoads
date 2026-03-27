#pragma once

#include "services/auth_service.hpp"
#include "services/token_service.hpp"

#include <httplib.h>

namespace controllers {

class AuthController {
public:
    AuthController(services::AuthService& auth_service, const services::TokenService& token_service);

    void register_user(const httplib::Request& request, httplib::Response& response) const;
    void login_user(const httplib::Request& request, httplib::Response& response) const;
    void me(const httplib::Request& request, httplib::Response& response) const;
    void options_ok(const httplib::Request& request, httplib::Response& response) const;

private:
    [[nodiscard]] static nlohmann::json parse_body(const httplib::Request& request);
    [[nodiscard]] static std::string read_required_string(const nlohmann::json& body, const char* key);
    [[nodiscard]] static std::string extract_bearer_token(const httplib::Request& request);

    services::AuthService& auth_service_;
    const services::TokenService& token_service_;
};

}  // namespace controllers
