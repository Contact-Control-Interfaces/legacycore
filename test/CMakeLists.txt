cmake_minimum_required(VERSION 3.20)

# manual/  one-off tools that drive a real service and glove; compiled, never run by CI
# unit/    the automated suite (GoogleTest + CTest, JUnit reports); built with
#          -DCCI_BUILD_UNIT_TESTS=ON, which build.ps1 -IncludeTests sets
add_subdirectory(manual)

if(CCI_BUILD_UNIT_TESTS)
    add_subdirectory(unit)
endif()
