add_library(backend_core
    config/app_config.cpp
    config/database.cpp
    config/environment.cpp
    controllers/auth_controller.cpp
    repositories/pqxx_user_repository.cpp
    services/auth_service.cpp
    services/jwt_token_service.cpp
    utils/http_utils.cpp
)

target_include_directories(backend_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(backend_core
    PUBLIC
        CPPHTTPLIB_OPENSSL_SUPPORT
)

target_link_libraries(backend_core
    PUBLIC
        httplib::httplib
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        OpenSSL::SSL
        OpenSSL::Crypto
)

add_executable(backend main.cpp)
target_link_libraries(backend PRIVATE backend_core)
