cmake_minimum_required(VERSION 3.20)
project(cci)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(interfaces)
add_subdirectory(libraries)
add_subdirectory(tests)