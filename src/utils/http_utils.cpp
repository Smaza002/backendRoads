#include "utils/http_utils.hpp"

namespace utils {

void set_cors_headers(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
}

void send_json(httplib::Response& res, int status, const nlohmann::json& body) {
    set_cors_headers(res);
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

}  // namespace utils
