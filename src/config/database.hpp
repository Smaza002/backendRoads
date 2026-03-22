#pragma once

#include <pqxx/pqxx>

#include <string>

namespace config {

std::string database_url();
pqxx::connection make_connection();
void ensure_extensions();

}  // namespace config
