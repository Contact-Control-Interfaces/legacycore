cmake_minimum_required(VERSION 3.20)
project(test_service_client_listing)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC library_io_pipe library_packet)