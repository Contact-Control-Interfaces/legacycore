cmake_minimum_required(VERSION 3.20)
project(types C)

set(INCLUDE_DIR include)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE ${INCLUDE_DIR})