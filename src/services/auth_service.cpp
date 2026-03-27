#include "services/auth_service.hpp"

#include <stdexcept>

namespace services {

AuthService::AuthService(repositories::UserRepository& user_repository, const TokenService& token_service)
    : user_repository_(user_repository),
      token_service_(token_service) {}

RegisteredUser AuthService::register_user(std::string_view email, std::string_view password) {
    validate_credentials(email, password);

    const auto user = user_repository_.create(email, password);
    return RegisteredUser{user.id, user.email};
}

LoginResult AuthService::login_user(std::string_view email, std::string_view password) const {
    validate_credentials(email, password);

    const auto user = user_repository_.find_by_email(email);
    if (!user.has_value()) {
        throw std::runtime_error("Usuario no encontrado");
    }

    if (!user_repository_.verify_password(password, user->password_hash)) {
        throw std::runtime_error("Contrasena incorrecta");
    }

    return LoginResult{token_service_.create_token(UserIdentity{user->id, user->email})};
}

void AuthService::validate_credentials(std::string_view email, std::string_view password) {
    if (email.empty() || password.empty()) {
        throw std::runtime_error("Email y password son obligatorios");
    }
}

}  // namespace services
