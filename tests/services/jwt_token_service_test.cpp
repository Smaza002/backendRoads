#include "services/jwt_token_service.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <string>
#include <string_view>
#include <vector>

namespace {

using json = nlohmann::json;

std::string base64_url_encode(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    std::string encoded;
    encoded.resize(4 * ((input.size() + 2) / 3));

    const int length = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(encoded.data()),
        reinterpret_cast<const unsigned char*>(input.data()),
        static_cast<int>(input.size()));

    encoded.resize(length);

    for (char& character : encoded) {
        if (character == '+') {
            character = '-';
        } else if (character == '/') {
            character = '_';
        }
    }

    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }

    return encoded;
}

std::string hmac_sha256(std::string_view data, std::string_view secret) {
    unsigned int length = EVP_MAX_MD_SIZE;
    std::vector<unsigned char> digest(length);

    HMAC(
        EVP_sha256(),
        secret.data(),
        static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char*>(data.data()),
        data.size(),
        digest.data(),
        &length);

    return std::string(reinterpret_cast<char*>(digest.data()), length);
}

std::string sign_token(std::string_view secret, const json& payload, const json& header = json{{"alg", "HS256"}, {"typ", "JWT"}}) {
    const std::string header_part = base64_url_encode(header.dump());
    const std::string payload_part = base64_url_encode(payload.dump());
    const std::string signing_input = header_part + "." + payload_part;
    return signing_input + "." + base64_url_encode(hmac_sha256(signing_input, secret));
}

}  // namespace

TEST(JwtTokenServiceTest, RejectsEmptySecret) {
    EXPECT_THROW((void)services::JwtTokenService{""}, std::runtime_error);
}

TEST(JwtTokenServiceTest, CreatesAndVerifiesToken) {
    services::JwtTokenService service{"secret-value"};

    const auto token = service.create_token(services::UserIdentity{42, "user@example.com"});
    const auto payload = service.verify_token(token);

    EXPECT_EQ(payload.at("id"), 42);
    EXPECT_EQ(payload.at("email"), "user@example.com");
    EXPECT_TRUE(payload.contains("iat"));
    EXPECT_TRUE(payload.contains("exp"));
    EXPECT_LT(payload.at("iat").get<long long>(), payload.at("exp").get<long long>());
}

TEST(JwtTokenServiceTest, RejectsMalformedToken) {
    services::JwtTokenService service{"secret-value"};

    EXPECT_THROW((void)service.verify_token("invalid-token"), std::runtime_error);
}

TEST(JwtTokenServiceTest, RejectsTokenWithTamperedSignature) {
    services::JwtTokenService service{"secret-value"};

    std::string token = service.create_token(services::UserIdentity{42, "user@example.com"});
    token.back() = token.back() == 'a' ? 'b' : 'a';

    EXPECT_THROW((void)service.verify_token(token), std::runtime_error);
}

TEST(JwtTokenServiceTest, RejectsExpiredToken) {
    services::JwtTokenService service{"secret-value"};

    const auto token = sign_token("secret-value", {
        {"id", 42},
        {"email", "user@example.com"},
        {"iat", 1000},
        {"exp", 1001}
    });

    EXPECT_THROW((void)service.verify_token(token), std::runtime_error);
}

TEST(JwtTokenServiceTest, RejectsUnsupportedAlgorithm) {
    services::JwtTokenService service{"secret-value"};

    const auto token = sign_token(
        "secret-value",
        {{"id", 42}, {"email", "user@example.com"}, {"iat", 1000}, {"exp", 9999999999LL}},
        {{"alg", "none"}, {"typ", "JWT"}});

    EXPECT_THROW((void)service.verify_token(token), std::runtime_error);
}