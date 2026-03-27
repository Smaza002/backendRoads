#include "services/auth_service.hpp"

#include <gtest/gtest.h>

namespace {

class FakeUserRepository final : public repositories::UserRepository {
public:
    repositories::UserRecord create(std::string_view email, std::string_view) override {
        create_called = true;
        last_email = std::string{email};
        return created_user;
    }

    std::optional<repositories::UserRecord> find_by_email(std::string_view email) override {
        find_called = true;
        last_email = std::string{email};
        return found_user;
    }

    bool verify_password(std::string_view password, std::string_view password_hash) override {
        verify_called = true;
        last_password = std::string{password};
        last_password_hash = std::string{password_hash};
        return password_is_valid;
    }

    repositories::UserRecord created_user{7, "created@example.com", "hash"};
    std::optional<repositories::UserRecord> found_user;
    bool password_is_valid{false};
    bool create_called{false};
    bool find_called{false};
    bool verify_called{false};
    std::string last_email;
    std::string last_password;
    std::string last_password_hash;
};

class FakeTokenService final : public services::TokenService {
public:
    std::string create_token(const services::UserIdentity& user) const override {
        last_user = user;
        return token_to_return;
    }

    nlohmann::json verify_token(std::string_view) const override {
        return {};
    }

    mutable services::UserIdentity last_user{0, ""};
    std::string token_to_return{"test-token"};
};

}  // namespace

TEST(AuthServiceTest, RegistersUserThroughRepository) {
    FakeUserRepository repository;
    FakeTokenService token_service;
    services::AuthService service{repository, token_service};

    const auto user = service.register_user("user@example.com", "password");

    EXPECT_TRUE(repository.create_called);
    EXPECT_EQ(repository.last_email, "user@example.com");
    EXPECT_EQ(user.id, 7);
    EXPECT_EQ(user.email, "created@example.com");
}

TEST(AuthServiceTest, CreatesTokenWhenCredentialsAreValid) {
    FakeUserRepository repository;
    repository.found_user = repositories::UserRecord{9, "user@example.com", "stored-hash"};
    repository.password_is_valid = true;

    FakeTokenService token_service;
    services::AuthService service{repository, token_service};

    const auto result = service.login_user("user@example.com", "password");

    EXPECT_TRUE(repository.find_called);
    EXPECT_TRUE(repository.verify_called);
    EXPECT_EQ(repository.last_password, "password");
    EXPECT_EQ(repository.last_password_hash, "stored-hash");
    EXPECT_EQ(token_service.last_user.id, 9);
    EXPECT_EQ(token_service.last_user.email, "user@example.com");
    EXPECT_EQ(result.token, "test-token");
}

TEST(AuthServiceTest, ThrowsWhenUserDoesNotExist) {
    FakeUserRepository repository;
    FakeTokenService token_service;
    services::AuthService service{repository, token_service};

    EXPECT_THROW((void)service.login_user("user@example.com", "password"), std::runtime_error);
}

TEST(AuthServiceTest, ThrowsWhenPasswordIsInvalid) {
    FakeUserRepository repository;
    repository.found_user = repositories::UserRecord{9, "user@example.com", "stored-hash"};

    FakeTokenService token_service;
    services::AuthService service{repository, token_service};

    EXPECT_THROW((void)service.login_user("user@example.com", "password"), std::runtime_error);
}
