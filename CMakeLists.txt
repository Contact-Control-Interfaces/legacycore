cmake_minimum_required(VERSION 3.20)
project(cci_core)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(types)
add_subdirectory(cci)
add_subdirectory(ccic)
#add_subdirectory(test)
