#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace services {

struct UserIdentity {
    int id;
    std::string email;
};

class TokenService {
public:
    virtual ~TokenService() = default;

    virtual std::string create_token(const UserIdentity& user) const = 0;
    virtual nlohmann::json verify_token(std::string_view token) const = 0;
};

}  // namespace services
