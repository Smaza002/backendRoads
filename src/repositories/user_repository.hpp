#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace repositories {

struct UserRecord {
    int id;
    std::string email;
    std::string password_hash;
};

class UserRepository {
public:
    virtual ~UserRepository() = default;

    virtual UserRecord create(std::string_view email, std::string_view password) = 0;
    virtual std::optional<UserRecord> find_by_email(std::string_view email) = 0;
    virtual bool verify_password(std::string_view password, std::string_view password_hash) = 0;
};

}  // namespace repositories
