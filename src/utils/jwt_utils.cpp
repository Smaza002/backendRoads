#include "utils/jwt_utils.hpp"

#include "utils/http_utils.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

namespace utils {
namespace {

using json = nlohmann::json;

std::string jwt_secret() {
    const char* value = std::getenv("JWT_SECRET");
    if (value == nullptr || std::string(value).empty()) {
        throw std::runtime_error("JWT_SECRET no esta configurado");
    }
    return value;
}

std::string extract_bearer_token(const httplib::Request& req) {
    const auto header = req.get_header_value("Authorization");
    if (header.empty()) {
        return {};
    }

    const std::string prefix = "Bearer ";
    if (header.rfind(prefix, 0) == 0) {
        return header.substr(prefix.size());
    }

    const auto position = header.find(' ');
    if (position == std::string::npos || position + 1 >= header.size()) {
        return {};
    }

    return header.substr(position + 1);
}

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

    for (char& ch : encoded) {
        if (ch == '+') {
            ch = '-';
        } else if (ch == '/') {
            ch = '_';
        }
    }

    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }

    return encoded;
}

std::string base64_url_decode(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    std::string normalized = input;
    for (char& ch : normalized) {
        if (ch == '-') {
            ch = '+';
        } else if (ch == '_') {
            ch = '/';
        }
    }

    while (normalized.size() % 4 != 0) {
        normalized.push_back('=');
    }

    std::string decoded;
    decoded.resize((normalized.size() / 4) * 3);

    const int length = EVP_DecodeBlock(
        reinterpret_cast<unsigned char*>(decoded.data()),
        reinterpret_cast<const unsigned char*>(normalized.data()),
        static_cast<int>(normalized.size()));

    if (length < 0) {
        throw std::runtime_error("Token invalido");
    }

    std::size_t padding = 0;
    if (!normalized.empty() && normalized.back() == '=') {
        padding++;
    }
    if (normalized.size() > 1 && normalized[normalized.size() - 2] == '=') {
        padding++;
    }

    decoded.resize(static_cast<std::size_t>(length) - padding);
    return decoded;
}

std::string hmac_sha256(const std::string& data, const std::string& secret) {
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

bool secure_equals(const std::string& left, const std::string& right) {
    if (left.size() != right.size()) {
        return false;
    }

    return CRYPTO_memcmp(left.data(), right.data(), left.size()) == 0;
}

json decode_and_verify_payload(const std::string& token) {
    const auto first_dot = token.find('.');
    const auto second_dot = token.find('.', first_dot == std::string::npos ? first_dot : first_dot + 1);

    if (first_dot == std::string::npos || second_dot == std::string::npos) {
        throw std::runtime_error("Token invalido");
    }

    const std::string header_part = token.substr(0, first_dot);
    const std::string payload_part = token.substr(first_dot + 1, second_dot - first_dot - 1);
    const std::string signature_part = token.substr(second_dot + 1);

    const std::string signing_input = header_part + "." + payload_part;
    const std::string expected_signature = base64_url_encode(hmac_sha256(signing_input, jwt_secret()));

    if (!secure_equals(signature_part, expected_signature)) {
        throw std::runtime_error("Token invalido");
    }

    const json header = json::parse(base64_url_decode(header_part));
    if (!header.contains("alg") || header.at("alg") != "HS256") {
        throw std::runtime_error("Token invalido");
    }

    const json payload = json::parse(base64_url_decode(payload_part));

    if (!payload.contains("exp") || !payload.at("exp").is_number_integer()) {
        throw std::runtime_error("Token invalido");
    }

    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();

    if (payload.at("exp").get<long long>() <= now) {
        throw std::runtime_error("Token invalido");
    }

    return payload;
}

}  // namespace

std::string create_token(int user_id, const std::string& email) {
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();
    const auto expiration = now + (60 * 60 * 24 * 7);

    const json header = {
        {"alg", "HS256"},
        {"typ", "JWT"}
    };

    const json payload = {
        {"id", user_id},
        {"email", email},
        {"iat", now},
        {"exp", expiration}
    };

    const std::string header_part = base64_url_encode(header.dump());
    const std::string payload_part = base64_url_encode(payload.dump());
    const std::string signing_input = header_part + "." + payload_part;
    const std::string signature_part = base64_url_encode(hmac_sha256(signing_input, jwt_secret()));

    return signing_input + "." + signature_part;
}

bool authorize_request(const httplib::Request& req, httplib::Response& res, nlohmann::json& user_payload) {
    const std::string token = extract_bearer_token(req);

    if (token.empty()) {
        send_json(res, 401, {{"error", "No autorizado"}});
        return false;
    }

    try {
        const json payload = decode_and_verify_payload(token);

        user_payload = {
            {"id", payload.at("id")},
            {"email", payload.at("email")}
        };

        if (payload.contains("iat")) {
            user_payload["iat"] = payload.at("iat");
        }

        if (payload.contains("exp")) {
            user_payload["exp"] = payload.at("exp");
        }

        return true;
    } catch (...) {
        send_json(res, 401, {{"error", "Token invalido"}});
        return false;
    }
}

}  // namespace utils
