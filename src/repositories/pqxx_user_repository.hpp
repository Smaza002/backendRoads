#pragma once

#include "config/database.hpp"
#include "repositories/user_repository.hpp"

namespace repositories {

class PqxxUserRepository final : public UserRepository {
public:
    explicit PqxxUserRepository(const config::Database& database);

    UserRecord create(std::string_view email, std::string_view password) override;
    std::optional<UserRecord> find_by_email(std::string_view email) override;
    bool verify_password(std::string_view password, std::string_view password_hash) override;

private:
    const config::Database& database_;
};

}  // namespace repositories
