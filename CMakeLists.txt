cmake_minimum_required(VERSION 3.10)
project(job_system)

set(CMAKE_CXX_STANDARD 14)
find_package(GTest REQUIRED)

add_executable(job_system main.cpp JobSystem.h)
target_link_libraries(job_system pthread)

add_executable(unit_tests tests/Tests.cpp)
target_link_libraries(unit_tests gtest gmock gtest_main pthread)
add_test(NAME "Job system tests" COMMAND unit_tests)