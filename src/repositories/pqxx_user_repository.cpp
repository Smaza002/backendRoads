#include "repositories/pqxx_user_repository.hpp"

#include <pqxx/params>

namespace repositories {

PqxxUserRepository::PqxxUserRepository(const config::Database& database)
    : database_(database) {}

UserRecord PqxxUserRepository::create(std::string_view email, std::string_view password) {
    auto connection = database_.make_connection();
    pqxx::work transaction{connection};

    const auto result = transaction.exec(
        "INSERT INTO users (email, password) "
        "VALUES ($1, crypt($2, gen_salt('bf', 10))) "
        "RETURNING id, email, password",
        pqxx::params{email, password});

    const auto row = result.one_row();
    UserRecord user{
        row["id"].as<int>(),
        row["email"].c_str(),
        row["password"].c_str()
    };

    transaction.commit();
    return user;
}

std::optional<UserRecord> PqxxUserRepository::find_by_email(std::string_view email) {
    auto connection = database_.make_connection();
    pqxx::work transaction{connection};

    const auto result = transaction.exec(
        "SELECT id, email, password FROM users WHERE email = $1",
        pqxx::params{email});

    if (result.empty()) {
        return std::nullopt;
    }

    const auto row = result.front();
    return UserRecord{
        row["id"].as<int>(),
        row["email"].c_str(),
        row["password"].c_str()
    };
}

bool PqxxUserRepository::verify_password(std::string_view password, std::string_view password_hash) {
    auto connection = database_.make_connection();
    pqxx::work transaction{connection};

    const auto result = transaction.exec(
        "WITH pw AS (SELECT $2::text AS p) "
        "SELECT (crypt($1, p) = p) "
        "   OR (crypt($1, replace(p, '$2b$', '$2y$')) = replace(p, '$2b$', '$2y$')) "
        "   OR (crypt($1, replace(p, '$2b$', '$2a$')) = replace(p, '$2b$', '$2a$')) "
        "AS valid_password FROM pw",
        pqxx::params{password, password_hash});

    return result.one_row()["valid_password"].as<bool>();
}

}  // namespace repositories
