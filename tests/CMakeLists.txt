add_executable(backend_tests
    app_config_test.cpp
    auth_service_test.cpp
)

target_link_libraries(backend_tests
    PRIVATE
        backend_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(backend_tests)
