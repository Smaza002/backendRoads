#include "config/database.hpp"

namespace config {

Database::Database(std::string connection_string)
    : connection_string_(std::move(connection_string)) {}

pqxx::connection Database::make_connection() const {
    return pqxx::connection{connection_string_};
}

void Database::ensure_extensions() const {
    auto connection = make_connection();
    pqxx::work transaction{connection};
    transaction.exec("CREATE EXTENSION IF NOT EXISTS pgcrypto").no_rows();
    transaction.commit();
}

}  // namespace config
