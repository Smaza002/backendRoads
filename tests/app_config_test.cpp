#include "config/app_config.hpp"

#include <gtest/gtest.h>

#include <unordered_map>

namespace {

class FakeEnvironment final : public config::Environment {
public:
    FakeEnvironment(std::initializer_list<std::pair<const std::string, std::string>> values)
        : values_(values) {}

    std::optional<std::string> get(std::string_view key) const override {
        if (const auto it = values_.find(std::string{key}); it != values_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::unordered_map<std::string, std::string> values_;
};

}  // namespace

TEST(AppConfigTest, ReadsCompleteConfiguration) {
    const FakeEnvironment environment{
        {"HOST", "127.0.0.1"},
        {"PORT", "9090"},
        {"HTTPS_ENABLED", "true"},
        {"HTTPS_PORT", "9443"},
        {"HTTPS_CERT_FILE", "/tmp/cert.pem"},
        {"HTTPS_KEY_FILE", "/tmp/key.pem"},
        {"DATABASE_URL", "postgres://user:pass@db/app"},
        {"MIGRATION_DATABASE_URL", "postgres://user:pass@migrations-db/app"},
        {"JWT_SECRET", "secret-value"}
    };

    const auto config = config::AppConfig::from_environment(environment);

    EXPECT_EQ(config.host, "127.0.0.1");
    EXPECT_EQ(config.http_port, 9090);
    EXPECT_TRUE(config.https_enabled);
    EXPECT_EQ(config.https_port, 9443);
    EXPECT_EQ(config.active_port(), 9443);
    EXPECT_EQ(config.https_cert_file, "/tmp/cert.pem");
    EXPECT_EQ(config.https_key_file, "/tmp/key.pem");
    EXPECT_EQ(config.database_url, "postgres://user:pass@db/app?sslmode=require");
    EXPECT_EQ(config.migration_database_url, "postgres://user:pass@migrations-db/app?sslmode=require");
    EXPECT_EQ(config.jwt_secret, "secret-value");
}

TEST(AppConfigTest, KeepsExistingSslMode) {
    const FakeEnvironment environment{
        {"DATABASE_URL", "postgres://user:pass@db/app?sslmode=disable"},
        {"MIGRATION_DATABASE_URL", "postgres://user:pass@migrations-db/app?sslmode=disable"},
        {"JWT_SECRET", "secret-value"}
    };

    const auto config = config::AppConfig::from_environment(environment);
    EXPECT_EQ(config.database_url, "postgres://user:pass@db/app?sslmode=disable");
    EXPECT_EQ(config.migration_database_url, "postgres://user:pass@migrations-db/app?sslmode=disable");
}

TEST(AppConfigTest, FallsBackToDatabaseUrlForMigrations) {
    const FakeEnvironment environment{
        {"DATABASE_URL", "postgres://user:pass@db/app"},
        {"JWT_SECRET", "secret-value"}
    };

    const auto config = config::AppConfig::from_environment(environment);
    EXPECT_EQ(config.migration_database_url, config.database_url);
}

TEST(AppConfigTest, ThrowsWhenRequiredValuesAreMissing) {
    const FakeEnvironment environment{{"JWT_SECRET", "secret-value"}};
    EXPECT_THROW((void)config::AppConfig::from_environment(environment), std::runtime_error);
}
