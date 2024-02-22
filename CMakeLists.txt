cmake_minimum_required(VERSION 3.20)
project(ccic)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(lib)
add_subdirectory(test)