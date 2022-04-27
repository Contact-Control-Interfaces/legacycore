cmake_minimum_required(VERSION 3.20)
project(contactci_lib)

set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE ./include ../comms/src/include)