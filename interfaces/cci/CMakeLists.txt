cmake_minimum_required(VERSION 3.20)
project(interface_cci)

add_library(${PROJECT_NAME} INTERFACE)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME} INTERFACE include)

target_link_libraries(${PROJECT_NAME} INTERFACE interface_io)