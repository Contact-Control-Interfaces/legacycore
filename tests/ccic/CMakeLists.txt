cmake_minimum_required(VERSION 3.20)
project(test_ccic)

set(CMAKE_C_STANDARD 99)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} library_comms_stdout library_ccic)
