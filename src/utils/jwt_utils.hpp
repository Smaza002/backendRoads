#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <string>

namespace utils {

std::string create_token(int user_id, const std::string& email);
bool authorize_request(const httplib::Request& req, httplib::Response& res, nlohmann::json& user_payload);

}  // namespace utils
