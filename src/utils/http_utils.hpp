#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace utils {

void set_cors_headers(const httplib::Request& req, httplib::Response& res);
void send_json(const httplib::Request& req, httplib::Response& res, int status, const nlohmann::json& body);

}  // namespace utils
