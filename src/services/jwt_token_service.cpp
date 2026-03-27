#include "services/jwt_token_service.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <stdexcept>
#include <vector>

namespace services {
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

std::string base64_url_decode(std::string input) {
    if (input.empty()) {
        return {};
    }

    for (char& character : input) {
        if (character == '-') {
            character = '+';
        } else if (character == '_') {
            character = '/';
        }
    }

    while (input.size() % 4 != 0) {
        input.push_back('=');
    }

    std::string decoded((input.size() / 4) * 3, '\0');
    const int length = EVP_DecodeBlock(
        reinterpret_cast<unsigned char*>(decoded.data()),
        reinterpret_cast<const unsigned char*>(input.data()),
        static_cast<int>(input.size()));

    if (length < 0) {
        throw std::runtime_error("Token invalido");
    }

    auto padding = std::size_t{0};
    if (!input.empty() && input.back() == '=') {
        ++padding;
    }
    if (input.size() > 1 && input[input.size() - 2] == '=') {
        ++padding;
    }

    decoded.resize(static_cast<std::size_t>(length) - padding);
    return decoded;
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

bool secure_equals(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) {
        return false;
    }

    return CRYPTO_memcmp(left.data(), right.data(), left.size()) == 0;
}

}  // namespace

JwtTokenService::JwtTokenService(std::string secret)
    : secret_(std::move(secret)) {
    if (secret_.empty()) {
        throw std::runtime_error("JWT_SECRET no esta configurado");
    }
}

std::string JwtTokenService::create_token(const UserIdentity& user) const {
    const auto issued_at = now_as_epoch_seconds();
    const auto expiration = issued_at + ttl_.count();

    const json header = {
        {"alg", "HS256"},
        {"typ", "JWT"}
    };

    const json payload = {
        {"id", user.id},
        {"email", user.email},
        {"iat", issued_at},
        {"exp", expiration}
    };

    const std::string header_part = base64_url_encode(header.dump());
    const std::string payload_part = base64_url_encode(payload.dump());
    const std::string signing_input = header_part + "." + payload_part;
    const std::string signature = base64_url_encode(hmac_sha256(signing_input, secret_));
    return signing_input + "." + signature;
}

nlohmann::json JwtTokenService::verify_token(std::string_view token) const {
    const auto first_dot = token.find('.');
    const auto second_dot = token.find('.', first_dot == std::string_view::npos ? first_dot : first_dot + 1);

    if (first_dot == std::string_view::npos || second_dot == std::string_view::npos) {
        throw std::runtime_error("Token invalido");
    }

    const std::string header_part{token.substr(0, first_dot)};
    const std::string payload_part{token.substr(first_dot + 1, second_dot - first_dot - 1)};
    const std::string signature_part{token.substr(second_dot + 1)};
    const std::string signing_input = header_part + "." + payload_part;
    const auto expected_signature = base64_url_encode(hmac_sha256(signing_input, secret_));

    if (!secure_equals(signature_part, expected_signature)) {
        throw std::runtime_error("Token invalido");
    }

    const auto header = json::parse(base64_url_decode(header_part));
    if (!header.contains("alg") || header.at("alg") != "HS256") {
        throw std::runtime_error("Token invalido");
    }

    const auto payload = json::parse(base64_url_decode(payload_part));
    if (!payload.contains("exp") || !payload.at("exp").is_number_integer()) {
        throw std::runtime_error("Token invalido");
    }

    if (payload.at("exp").get<long long>() <= now_as_epoch_seconds()) {
        throw std::runtime_error("Token invalido");
    }

    return payload;
}

long long JwtTokenService::now_as_epoch_seconds() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();
}

}  // namespace services
