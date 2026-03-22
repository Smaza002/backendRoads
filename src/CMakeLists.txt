add_executable(backend
    main.cpp
    config/database.cpp
    controllers/auth_controller.cpp
    services/auth_service.cpp
    utils/http_utils.cpp
    utils/jwt_utils.cpp
)

target_include_directories(backend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(backend
    PRIVATE
        CPPHTTPLIB_OPENSSL_SUPPORT
)

target_link_libraries(backend
    PRIVATE
        httplib::httplib
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        OpenSSL::SSL
        OpenSSL::Crypto
)
