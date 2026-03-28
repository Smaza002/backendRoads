#pragma once

#include "config/environment.hpp"

#include <string>

namespace config {

struct AppConfig {
    std::string host{"0.0.0.0"};
    int http_port{8080};
    int https_port{8443};
    bool https_enabled{false};
    std::string https_cert_file;
    std::string https_key_file;
    std::string database_url;
    std::string migration_database_url;
    std::string jwt_secret;

    [[nodiscard]] int active_port() const noexcept;

    static AppConfig from_environment(const Environment& environment);
};

}  // namespace config
