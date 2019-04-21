cmake_minimum_required(VERSION 3.10)
project(job_system)

set(CMAKE_CXX_STANDARD 14)
find_package(GTest REQUIRED)

add_library(job_system STATIC src/JobSystem.cpp include/JobSystem.hpp)
target_link_libraries(job_system pthread)
target_include_directories(job_system
        PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>"
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_executable(unit_tests tests/Tests.cpp)
target_link_libraries(unit_tests gtest gmock gtest_main pthread job_system)
add_test(NAME "Job system tests" COMMAND unit_tests)