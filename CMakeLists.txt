cmake_minimum_required(VERSION 3.25)

project(backend
    VERSION 0.2.0
    DESCRIPTION "C++20 auth backend for Render"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ProjectOptions)

find_package(httplib REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(libpqxx REQUIRED)
find_package(OpenSSL REQUIRED)

add_subdirectory(src)
