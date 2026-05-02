cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

project(BuilderSync VERSION 1.0.0)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "GEODE_SDK environment variable not set.\n"
        "Run: export GEODE_SDK=/Users/Shared/Geode/sdk"
    )
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# ── CPM package manager (auto-downloads on first configure) ────────────────
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)

# ── nlohmann/json ──────────────────────────────────────────────────────────
CPMAddPackage("gh:nlohmann/json@3.11.3")

# ── IXWebSocket (header-only WS client) ────────────────────────────────────
CPMAddPackage(
    NAME IXWebSocket
    GITHUB_REPOSITORY machinezone/IXWebSocket
    GIT_TAG v11.4.5
    OPTIONS
        "USE_TLS OFF"
        "IXWEBSOCKET_INSTALL OFF"
)

# ── Source files ────────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src/)

target_link_libraries(${PROJECT_NAME}
    geode-sdk
    nlohmann_json::nlohmann_json
    IXWebSocket::IXWebSocket
)

setup_geode_mod(${PROJECT_NAME})
