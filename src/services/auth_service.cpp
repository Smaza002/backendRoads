#include "services/auth_service.hpp"

#include "config/database.hpp"
#include "utils/jwt_utils.hpp"

#include <pqxx/pqxx>

#include <stdexcept>
#include <string>

namespace services {
namespace {

void validate_credentials(const std::string& email, const std::string& password) {
    if (email.empty() || password.empty()) {
        throw std::runtime_error("Email y password son obligatorios");
    }
}

}  // namespace

RegisteredUser register_user(const std::string& email, const std::string& password) {
    validate_credentials(email, password);

    auto connection = config::make_connection();
    pqxx::work tx(connection);

    const pqxx::row row = tx.exec_params1(
        "INSERT INTO users (email, password) "
        "VALUES ($1, crypt($2, gen_salt('bf', 10))) "
        "RETURNING id, email",
        email,
        password);

    RegisteredUser user{
        row["id"].as<int>(),
        row["email"].c_str()
    };

    tx.commit();
    return user;
}

LoginResult login_user(const std::string& email, const std::string& password) {
    validate_credentials(email, password);

    auto connection = config::make_connection();
    pqxx::work tx(connection);

    const pqxx::result users = tx.exec_params(
        "SELECT id, email, password FROM users WHERE email = $1",
        email);

    if (users.empty()) {
        throw std::runtime_error("Usuario no encontrado");
    }

    const pqxx::row user = users.front();
    const std::string password_hash = user["password"].c_str();

    const pqxx::row validation = tx.exec_params1(
        "SELECT crypt($1, $2) = $2 AS valid_password",
        password,
        password_hash);

    if (!validation["valid_password"].as<bool>()) {
        throw std::runtime_error("Contrase\u00f1a incorrecta");
    }

    LoginResult result{
        utils::create_token(user["id"].as<int>(), user["email"].c_str())
    };

    tx.commit();
    return result;
}

}  // namespace services
