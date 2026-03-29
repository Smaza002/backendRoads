#include "controllers/auth_controller.hpp"

#include "services/auth_service.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <optional>
#include <stdexcept>
#include <string>

namespace {

class FakeUserRepository final : public repositories::UserRepository {
public:
    repositories::UserRecord create(std::string_view email, std::string_view) override {
        create_called = true;
        last_email = std::string{email};
        return created_user;
    }

    std::optional<repositories::UserRecord> find_by_email(std::string_view email) override {
        find_called = true;
        last_email = std::string{email};
        return found_user;
    }

    bool verify_password(std::string_view password, std::string_view password_hash) override {
        verify_called = true;
        last_password = std::string{password};
        last_password_hash = std::string{password_hash};
        return password_is_valid;
    }

    repositories::UserRecord created_user{7, "created@example.com", "hash"};
    std::optional<repositories::UserRecord> found_user;
    bool password_is_valid{false};
    bool create_called{false};
    bool find_called{false};
    bool verify_called{false};
    std::string last_email;
    std::string last_password;
    std::string last_password_hash;
};

class FakeTokenService final : public services::TokenService {
public:
    std::string create_token(const services::UserIdentity& user) const override {
        last_created_user = user;
        return token_to_return;
    }

    nlohmann::json verify_token(std::string_view token) const override {
        last_verified_token = std::string{token};
        if (verify_should_throw) {
            throw std::runtime_error("Token invalido");
        }
        return payload_to_return;
    }

    mutable services::UserIdentity last_created_user{0, ""};
    mutable std::string last_verified_token;
    nlohmann::json payload_to_return{{"id", 9}, {"email", "user@example.com"}, {"iat", 1000}, {"exp", 2000}};
    std::string token_to_return{"signed-token"};
    bool verify_should_throw{false};
};

nlohmann::json parse_json_body(const httplib::Response& response) {
    return nlohmann::json::parse(response.body);
}

}  // namespace

TEST(AuthControllerTest, RegistersUserAndReturnsCreatedResponse) {
    FakeUserRepository repository;
    FakeTokenService token_service;
    services::AuthService auth_service{repository, token_service};
    controllers::AuthController controller{auth_service, token_service};

    httplib::Request request;
    request.body = R"({"email":"user@example.com","password":"secret"})";
    httplib::Response response;

    controller.register_user(request, response);

    const auto body = parse_json_body(response);
    EXPECT_EQ(response.status, 201);
    EXPECT_TRUE(body.at("success"));
    EXPECT_EQ(body.at("data").at("id"), 7);
    EXPECT_EQ(body.at("data").at("email"), "created@example.com");
}

TEST(AuthControllerTest, ReturnsBadRequestWhenRegistrationBodyIsInvalid) {
    FakeUserRepository repository;
    FakeTokenService token_service;
    services::AuthService auth_service{repository, token_service};
    controllers::AuthController controller{auth_service, token_service};

    httplib::Request request;
    request.body = R"({"email":"user@example.com"})";
    httplib::Response response;

    controller.register_user(request, response);

    const auto body = parse_json_body(response);
    EXPECT_EQ(response.status, 400);
    EXPECT_FALSE(body.at("success"));
    EXPECT_EQ(body.at("message"), "Error al registrar el usuario");
}

TEST(AuthControllerTest, LogsUserInAndReturnsToken) {
    FakeUserRepository repository;
    repository.found_user = repositories::UserRecord{9, "user@example.com", "stored-hash"};
    repository.password_is_valid = true;

    FakeTokenService token_service;
    services::AuthService auth_service{repository, token_service};
    controllers::AuthController controller{auth_service, token_service};

    httplib::Request request;
    request.body = R"({"email":"user@example.com","password":"secret"})";
    httplib::Response response;

    controller.login_user(request, response);

    const auto body = parse_json_body(response);
    EXPECT_EQ(response.status, 200);
    EXPECT_TRUE(body.at("success"));
    EXPECT_EQ(body.at("token"), "signed-token");
}

TEST(AuthControllerTest, ReturnsUnauthorizedWhenLoginFails) {
    FakeUserRepository repository;
    FakeTokenService token_service;
    services::AuthService auth_service{repository, token_service};
    controllers::AuthController controller{auth_service, token_service};

    httplib::Request request;
    request.body = R"({"email":"user@example.com","password":"secret"})";
    httplib::Response response;

    controller.login_user(request, response);

    const auto body = parse_json_body(response);
    EXPECT_EQ(response.status, 401);
    EXPECT_FALSE(body.at("success"));
    EXPECT_EQ(body.at("message"), "Error en el login");
}

TEST(AuthControllerTest, ReturnsUnauthorizedWhenTokenIsMissing) {
    FakeUserRepository repository;
    FakeTokenService token_service;
    services::AuthService auth_service{repository, token_service};
    controllers::AuthController controller{auth_service, token_service};

    httplib::Request request;
    httplib::Response response;

    controller.me(request, response);

    const auto body = parse_json_body(response);
    EXPECT_EQ(response.status, 401);
    EXPECT_EQ(body.at("error"), "No autorizado");
}

TEST(AuthControllerTest, ReturnsCurrentUserWhenTokenIsValid) {
    FakeUserRepository repository;
    FakeTokenService token_service;
    services::AuthService auth_service{repository, token_service};
    controllers::AuthController controller{auth_service, token_service};

    httplib::Request request;
    request.headers.emplace("Authorization", "Bearer signed-token");
    httplib::Response response;

    controller.me(request, response);

    const auto body = parse_json_body(response);
    EXPECT_EQ(response.status, 200);
    EXPECT_TRUE(body.at("success"));
    EXPECT_EQ(body.at("user").at("id"), 9);
    EXPECT_EQ(body.at("user").at("email"), "user@example.com");
    EXPECT_EQ(token_service.last_verified_token, "signed-token");
}

TEST(AuthControllerTest, ReturnsUnauthorizedWhenTokenVerificationFails) {
    FakeUserRepository repository;
    FakeTokenService token_service;
    token_service.verify_should_throw = true;
    services::AuthService auth_service{repository, token_service};
    controllers::AuthController controller{auth_service, token_service};

    httplib::Request request;
    request.headers.emplace("Authorization", "Bearer signed-token");
    httplib::Response response;

    controller.me(request, response);

    const auto body = parse_json_body(response);
    EXPECT_EQ(response.status, 401);
    EXPECT_EQ(body.at("error"), "Token invalido");
}

TEST(AuthControllerTest, HandlesPreflightRequests) {
    FakeUserRepository repository;
    FakeTokenService token_service;
    services::AuthService auth_service{repository, token_service};
    controllers::AuthController controller{auth_service, token_service};

    httplib::Request request;
    request.headers.emplace("Origin", "http://localhost:8100");
    httplib::Response response;

    controller.options_ok(request, response);

    EXPECT_EQ(response.status, 204);
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Origin"), "http://localhost:8100");
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Methods"), "GET, POST, OPTIONS");
}