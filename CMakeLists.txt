cmake_minimum_required(VERSION 3.20)
project(cci_core)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/../deploy/${CMAKE_BUILD_TYPE}/")

# Automated tests in test/unit (GoogleTest + CTest). build.ps1 -IncludeTests turns this on.
option(CCI_BUILD_UNIT_TESTS "Build the automated tests in test/unit (needs GoogleTest)" OFF)
if(CCI_BUILD_UNIT_TESTS)
    enable_testing()
endif()

add_subdirectory(types)
add_subdirectory(cci)
add_subdirectory(ccic)
add_subdirectory(test)
