cmake_minimum_required(VERSION 3.20)
project(test_cci)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC library_comms_stdout library_cci)
