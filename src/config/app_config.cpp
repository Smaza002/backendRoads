#include "config/app_config.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace config {
namespace {

std::string get_or_default(const Environment& environment, std::string_view key, std::string fallback) {
    if (const auto value = environment.get(key); value.has_value()) {
        return *value;
    }
    return fallback;
}

std::string require_value(const Environment& environment, std::string_view key) {
    const auto value = environment.get(key);
    if (!value.has_value() || value->empty()) {
        throw std::runtime_error(std::string{key} + " no esta configurado");
    }
    return *value;
}

bool is_truthy(std::string value) {
    std::ranges::transform(value, value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });

    return value == "1" || value == "true" || value == "yes" || value == "on";
}

int parse_port(std::string_view value, int fallback) {
    try {
        return std::stoi(std::string{value});
    } catch (...) {
        return fallback;
    }
}

std::string normalize_database_url(std::string url) {
    if (url.find("sslmode=") == std::string::npos) {
        url += (url.find('?') == std::string::npos) ? "?sslmode=require" : "&sslmode=require";
    }

    return url;
}

}  // namespace

int AppConfig::active_port() const noexcept {
    return https_enabled ? https_port : http_port;
}

AppConfig AppConfig::from_environment(const Environment& environment) {
    AppConfig config;
    config.host = get_or_default(environment, "HOST", "0.0.0.0");
    config.http_port = parse_port(get_or_default(environment, "PORT", "8080"), 8080);
    config.https_port = parse_port(get_or_default(environment, "HTTPS_PORT", "8443"), 8443);
    config.https_enabled = is_truthy(get_or_default(environment, "HTTPS_ENABLED", ""));
    config.https_cert_file = get_or_default(environment, "HTTPS_CERT_FILE", config.https_enabled ? "cert.pem" : "");
    config.https_key_file = get_or_default(environment, "HTTPS_KEY_FILE", config.https_enabled ? "key.pem" : "");
    config.database_url = normalize_database_url(require_value(environment, "DATABASE_URL"));

    if (const auto migration_value = environment.get("MIGRATION_DATABASE_URL"); migration_value.has_value() && !migration_value->empty()) {
        config.migration_database_url = normalize_database_url(*migration_value);
    } else {
        config.migration_database_url = config.database_url;
    }

    config.jwt_secret = require_value(environment, "JWT_SECRET");
    return config;
}

}  // namespace config
