#pragma once

#include "repositories/user_repository.hpp"
#include "services/token_service.hpp"

#include <string>
#include <string_view>

namespace services {

struct RegisteredUser {
    int id;
    std::string email;
};

struct LoginResult {
    std::string token;
};

class AuthService {
public:
    AuthService(repositories::UserRepository& user_repository, const TokenService& token_service);

    RegisteredUser register_user(std::string_view email, std::string_view password);
    LoginResult login_user(std::string_view email, std::string_view password) const;

private:
    static void validate_credentials(std::string_view email, std::string_view password);

    repositories::UserRepository& user_repository_;
    const TokenService& token_service_;
};

}  // namespace services
