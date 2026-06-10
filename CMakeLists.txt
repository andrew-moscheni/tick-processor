cmake_minimum_required(VERSION 3.14)
project(TickProcessor VERSION 1.0)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# turn on warnings
add_compile_options(-Wall -Wextra -O2)

add_executable(tick_processor src/main.cpp src/parser.cpp)
target_include_directories(tick_processor PRIVATE include)

# test setup
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# test the executable
add_executable(run_tests tests/test_main.cpp src/parser.cpp)
target_link_libraries(run_tests ${GTEST_LIBRARIES} pthread)
target_include_directories(run_tests PRIVATE include)
