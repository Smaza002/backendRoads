#pragma once

#include <string>

namespace services {

struct RegisteredUser {
    int id;
    std::string email;
};

struct LoginResult {
    std::string token;
};

RegisteredUser register_user(const std::string& email, const std::string& password);
LoginResult login_user(const std::string& email, const std::string& password);

}  // namespace services
