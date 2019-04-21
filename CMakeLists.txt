cmake_minimum_required(VERSION 3.10)
project(job_system)

set(CMAKE_CXX_STANDARD 14)

add_executable(job_system main.cpp JobSystem.h)
target_link_libraries(job_system pthread)