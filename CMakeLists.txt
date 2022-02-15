cmake_minimum_required(VERSION 3.20)
project(contactci_lib_tester)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(lib)


cmake_path(SET LIB_DIR "./lib")
cmake_path(SET LIB_INCLUDE_DIR ${LIB_DIR}/src/include)

include_directories(${LIB_INCLUDE_DIR})

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} contactci_lib)