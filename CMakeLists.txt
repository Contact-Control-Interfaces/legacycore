cmake_minimum_required(VERSION 3.20)
project(cci_core)

# X.Y.Z the repository is working towards (tools/resolve-version.ps1 reads the
# same file). On Linux it sets the libraries' VERSION and SOVERSION.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" CCI_VERSION LIMIT_COUNT 1)
if(NOT CCI_VERSION MATCHES "^([0-9]+)\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must hold X.Y.Z (got '${CCI_VERSION}')")
endif()
set(CCI_VERSION_MAJOR "${CMAKE_MATCH_1}")

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

if(WIN32)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/../deploy/${CMAKE_BUILD_TYPE}/")
endif()

# Automated tests in test/unit (GoogleTest + CTest). build.ps1 -IncludeTests turns this on.
option(CCI_BUILD_UNIT_TESTS "Build the automated tests in test/unit (needs GoogleTest)" OFF)
if(CCI_BUILD_UNIT_TESTS)
    enable_testing()
endif()

add_subdirectory(types)
add_subdirectory(cci)
add_subdirectory(ccic)
add_subdirectory(test)

# -----------------------------------------------------------------------------
# Linux install tree, packaged by build.sh --package (debian/ and a tarball):
#   lib/libcci.so.X.Y.Z, libccic.so.X.Y.Z   runtime, SONAME libcci.so.X / libccic.so.X
#   lib/libcci.so, libccic.so               unversioned links, for linking
#   include/contactci.h                     the C API
#   include/ccic/*.h                        types the C API uses
#   include/cci/*.h                         the C++ headers
# Windows keeps its own install rules in each subdirectory.
# -----------------------------------------------------------------------------
if(NOT WIN32)
    include(GNUInstallDirs)

    set_target_properties(cci ccic PROPERTIES
        VERSION "${CCI_VERSION}"
        SOVERSION "${CCI_VERSION_MAJOR}"
        PUBLIC_HEADER "")

    install(TARGETS cci ccic
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    install(FILES ccic/include/contactci.h
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(DIRECTORY types/include/ccic cci/include/cci
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            FILES_MATCHING PATTERN "*.h")
endif()
