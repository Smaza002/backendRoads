#include "utils/http_utils.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <optional>
#include <string>

namespace {

class ScopedEnvVar {
public:
    ScopedEnvVar(std::string key, std::optional<std::string> value)
        : key_(std::move(key)),
          original_(read(key_)) {
        write(value);
    }

    ~ScopedEnvVar() {
        write(original_);
    }

private:
    static std::optional<std::string> read(const std::string& key) {
        if (const char* value = std::getenv(key.c_str()); value != nullptr) {
            return std::string{value};
        }
        return std::nullopt;
    }

    void write(const std::optional<std::string>& value) const {
        if (value.has_value()) {
            setenv(key_.c_str(), value->c_str(), 1);
        } else {
            unsetenv(key_.c_str());
        }
    }

    std::string key_;
    std::optional<std::string> original_;
};

}  // namespace

TEST(HttpUtilsTest, SendJsonSetsResponseStatusAndContent) {
    ScopedEnvVar allowed_origins{"ALLOWED_ORIGINS", std::nullopt};

    httplib::Request request;
    httplib::Response response;

    utils::send_json(request, response, 202, {{"success", true}});

    EXPECT_EQ(response.status, 202);
    EXPECT_EQ(response.body, R"({"success":true})");
    EXPECT_EQ(response.get_header_value("Content-Type"), "application/json");
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Headers"), "Content-Type, Authorization");
}

TEST(HttpUtilsTest, SetCorsHeadersUsesConfiguredAllowedOrigin) {
    ScopedEnvVar allowed_origins{"ALLOWED_ORIGINS", "https://app.example.com, https://admin.example.com"};

    httplib::Request request;
    request.headers.emplace("Origin", "https://admin.example.com");
    request.headers.emplace("Access-Control-Request-Headers", "X-Test-Header");
    httplib::Response response;

    utils::set_cors_headers(request, response);

    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Origin"), "https://admin.example.com");
    EXPECT_EQ(response.get_header_value("Vary"), "Origin");
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Headers"), "X-Test-Header");
}

TEST(HttpUtilsTest, SetCorsHeadersOmitsOriginWhenItIsNotAllowed) {
    ScopedEnvVar allowed_origins{"ALLOWED_ORIGINS", "https://app.example.com"};

    httplib::Request request;
    request.headers.emplace("Origin", "https://forbidden.example.com");
    httplib::Response response;

    utils::set_cors_headers(request, response);

    EXPECT_TRUE(response.get_header_value("Access-Control-Allow-Origin").empty());
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Methods"), "GET, POST, OPTIONS");
    EXPECT_EQ(response.get_header_value("Access-Control-Max-Age"), "600");
}

TEST(HttpUtilsTest, SetCorsHeadersSupportsWildcardOrigin) {
    ScopedEnvVar allowed_origins{"ALLOWED_ORIGINS", "*"};

    httplib::Request request;
    request.headers.emplace("Origin", "https://anywhere.example.com");
    httplib::Response response;

    utils::set_cors_headers(request, response);

    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Origin"), "https://anywhere.example.com");
}