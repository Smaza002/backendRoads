#include "config/database.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace config {

std::string database_url() {
    const char* value = std::getenv("DATABASE_URL");
    if (value == nullptr || std::string(value).empty()) {
        throw std::runtime_error("DATABASE_URL no esta configurado");
    }

    std::string url = value;
    if (url.find("sslmode=") == std::string::npos) {
        url += (url.find('?') == std::string::npos) ? "?sslmode=require" : "&sslmode=require";
    }

    return url;
}

pqxx::connection make_connection() {
    return pqxx::connection(database_url());
}

void ensure_extensions() {
    auto connection = make_connection();
    pqxx::work tx(connection);
    tx.exec("CREATE EXTENSION IF NOT EXISTS pgcrypto");
    tx.commit();
}

}  // namespace config
