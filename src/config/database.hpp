#pragma once

#include <pqxx/pqxx>

#include <string>

namespace config {

class Database {
public:
    explicit Database(std::string connection_string);

    [[nodiscard]] pqxx::connection make_connection() const;
    void ensure_extensions() const;

private:
    std::string connection_string_;
};

}  // namespace config
