cmake_minimum_required(VERSION 3.20)
project(test_cci CXX)

add_executable(${PROJECT_NAME} main.cpp)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_link_libraries(${PROJECT_NAME} PUBLIC cci_static)
