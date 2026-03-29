add_executable(backend_tests
    config/app_config_test.cpp
    config/environment_test.cpp
    controllers/auth_controller_test.cpp
    services/auth_service_test.cpp
    services/jwt_token_service_test.cpp
    utils/http_utils_test.cpp
)

target_link_libraries(backend_tests
    PRIVATE
        backend_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(backend_tests)
