#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace utils {

void set_cors_headers(httplib::Response& res);
void send_json(httplib::Response& res, int status, const nlohmann::json& body);

}  // namespace utils
