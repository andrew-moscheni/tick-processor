cmake_minimum_required(VERSION 3.15)
project(alpaca_terminal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

find_package(nlohmann_json QUIET)
if (NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_executable(alpaca_terminal alpaca_terminal.cpp)

target_link_libraries(alpaca_terminal
    PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
        Threads::Threads
)

find_package(Threads REQUIRED)
target_link_libraries(alpaca_terminal PRIVATE Threads::Threads)