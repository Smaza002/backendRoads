#include "utils/http_utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace utils {
namespace {

std::string trim(std::string value) {
    const auto not_space = [](unsigned char character) {
        return !std::isspace(character);
    };

    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::vector<std::string> default_allowed_origins() {
    return {
        "http://localhost:8100",
    };
}

std::vector<std::string> parse_allowed_origins() {
    const char* value = std::getenv("ALLOWED_ORIGINS");
    if (value == nullptr || std::string_view{value}.empty()) {
        return default_allowed_origins();
    }

    std::vector<std::string> origins;
    std::stringstream stream{value};
    std::string item;

    while (std::getline(stream, item, ',')) {
        item = trim(std::move(item));
        if (!item.empty()) {
            origins.push_back(std::move(item));
        }
    }

    if (origins.empty()) {
        return default_allowed_origins();
    }

    return origins;
}

std::optional<std::string> resolve_allowed_origin(const httplib::Request& req) {
    const auto origin = req.get_header_value("Origin");
    if (origin.empty()) {
        return std::nullopt;
    }

    const auto allowed_origins = parse_allowed_origins();
    if (std::ranges::find(allowed_origins, "*") != allowed_origins.end()) {
        return origin;
    }

    if (std::ranges::find(allowed_origins, origin) != allowed_origins.end()) {
        return origin;
    }

    return std::nullopt;
}

}  // namespace

void set_cors_headers(const httplib::Request& req, httplib::Response& res) {
    if (const auto origin = resolve_allowed_origin(req); origin.has_value()) {
        res.set_header("Access-Control-Allow-Origin", origin->c_str());
        res.set_header("Vary", "Origin");
    }

    const auto requested_headers = req.get_header_value("Access-Control-Request-Headers");
    if (!requested_headers.empty()) {
        res.set_header("Access-Control-Allow-Headers", requested_headers.c_str());
    } else {
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }

    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Max-Age", "600");
}

void send_json(const httplib::Request& req, httplib::Response& res, int status, const nlohmann::json& body) {
    set_cors_headers(req, res);
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

}  // namespace utils
