#pragma once

#include "services/token_service.hpp"

#include <chrono>
#include <string>

namespace services {

class JwtTokenService final : public TokenService {
public:
    explicit JwtTokenService(std::string secret);

    std::string create_token(const UserIdentity& user) const override;
    nlohmann::json verify_token(std::string_view token) const override;

private:
    [[nodiscard]] long long now_as_epoch_seconds() const;

    std::string secret_;
    std::chrono::seconds ttl_{std::chrono::hours{24 * 7}};
};

}  // namespace services
