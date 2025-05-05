cmake_minimum_required(VERSION 3.10)
project(DeribitTrader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Tell CMake to use vcpkg
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

include_directories(${CMAKE_SOURCE_DIR}/include)

find_path(WEBSOCKETPP_INCLUDE_DIR websocketpp/config/asio_client.hpp)

if(NOT WEBSOCKETPP_INCLUDE_DIR)
    message(FATAL_ERROR "WebSocket++ not found. Install it with 'sudo apt install libwebsocketpp-dev'.")
endif()

# Find packages
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(unofficial-uwebsockets CONFIG REQUIRED)
find_package(Boost REQUIRED system)

add_executable(DeribitTrader
    src/main.cpp
    src/auth.cpp
    src/trading.cpp
    src/utils.cpp
    src/DeribitPrivateWSClient.cpp
    src/DeribitWSClient.cpp
    include/ThreadPool.tpp
    src/CurlHandler.cpp
)

target_link_libraries(DeribitTrader
    PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
    Crow::Crow
    unofficial::uwebsockets::uwebsockets
    Boost::system
)
