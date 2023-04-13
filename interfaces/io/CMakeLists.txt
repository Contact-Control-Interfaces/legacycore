cmake_minimum_required(VERSION 3.20)
project(interface_io)

set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE ./include)
target_link_libraries(
        ${PROJECT_NAME}
        INTERFACE
        library_packet
)