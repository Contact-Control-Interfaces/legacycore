cmake_minimum_required(VERSION 3.20)
project(contactci_lib_tester)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(lib)
add_subdirectory(lib-impl)
add_subdirectory(clib-impl)
add_subdirectory(comms)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC lib/include)

target_link_libraries(${PROJECT_NAME} contactci_lib_impl contactci_comms_lib contactci_clib_impl)
