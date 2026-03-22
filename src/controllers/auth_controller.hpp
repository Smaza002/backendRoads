#pragma once

#include <httplib.h>

namespace controllers {

void register_user(const httplib::Request& req, httplib::Response& res);
void login_user(const httplib::Request& req, httplib::Response& res);
void me(const httplib::Request& req, httplib::Response& res);
void options_ok(const httplib::Request& req, httplib::Response& res);

}  // namespace controllers
