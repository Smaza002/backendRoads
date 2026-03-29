#include "config/environment.hpp"

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

TEST(ProcessEnvironmentTest, ReadsExistingEnvironmentVariable) {
    ScopedEnvVar env{"BACKEND_TEST_ENV", "configured-value"};
    config::ProcessEnvironment environment;

    const auto value = environment.get("BACKEND_TEST_ENV");

    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "configured-value");
}

TEST(ProcessEnvironmentTest, ReturnsNulloptForMissingEnvironmentVariable) {
    ScopedEnvVar env{"BACKEND_TEST_ENV", std::nullopt};
    config::ProcessEnvironment environment;

    EXPECT_FALSE(environment.get("BACKEND_TEST_ENV").has_value());
}