cmake_minimum_required(VERSION 3.21)
project(unit_tests CXX)

# Two test executables, run by tools/run-tests.ps1 (build.ps1 -IncludeTests,
# and the `test` job in CI), and also registered with CTest. Built only with
# -DCCI_BUILD_UNIT_TESTS=ON, so the shipped libcci/libccic build is exactly
# what it is without them.
#
#   unit_tests  the library's internal classes (framing, spinlock, events,
#               shared memory, haptic state, errors), compiled from cci/src
#   api_tests   the public C++ and C APIs, linked against the real libcci.dll
#               and libccic.dll, talking to a fake Windows service

include(GoogleTest)

find_package(Protobuf REQUIRED)
find_path(GTEST_INCLUDE_DIR gtest/gtest.h REQUIRED)
find_library(GTEST_LIBRARY NAMES libgtest.a gtest REQUIRED)
find_library(GTEST_MAIN_LIBRARY NAMES libgtest_main.a gtest_main REQUIRED)

# Statically linked like libcci.dll, so the tests need no MinGW runtime on PATH.
set(STATIC_LINK -static-libgcc -static-libstdc++ -static)

# The protocol messages, generated here for the tests' own use.
file(GLOB PROTOS ${CMAKE_SOURCE_DIR}/cci/packets/proto/*.proto)
protobuf_generate_cpp(TEST_PROTO_SRCS TEST_PROTO_HDRS ${PROTOS})

add_library(test_protos STATIC ${TEST_PROTO_SRCS} ${TEST_PROTO_HDRS})
target_include_directories(test_protos PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${Protobuf_INCLUDE_DIRS})
target_link_libraries(test_protos PUBLIC ${Protobuf_LIBRARIES})
set_property(TARGET test_protos PROPERTY CXX_STANDARD 20)

# The cci sources as a static library: the internal classes are not exported
# from libcci.dll, so unit_tests compiles them itself.
file(GLOB CCI_SOURCES ${CMAKE_SOURCE_DIR}/cci/src/*.cpp)
add_library(cci_internal STATIC ${CCI_SOURCES})
target_include_directories(cci_internal PUBLIC ${CMAKE_SOURCE_DIR}/cci/include)
target_link_libraries(cci_internal PUBLIC test_protos types)
target_compile_definitions(cci_internal PUBLIC CCI_API_EXPORT)
set_property(TARGET cci_internal PROPERTY CXX_STANDARD 20)

add_executable(unit_tests
    internal/error_test.cpp
    internal/spinlock_test.cpp
    internal/model_test.cpp
    internal/channel_test.cpp
    internal/named_event_test.cpp
    internal/shared_memory_test.cpp
    internal/haptic_state_test.cpp
    support/win_objects.cpp)
target_include_directories(unit_tests PRIVATE support ${GTEST_INCLUDE_DIR})
target_link_libraries(unit_tests PRIVATE cci_internal ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY})
target_link_options(unit_tests PRIVATE ${STATIC_LINK})
set_property(TARGET unit_tests PROPERTY CXX_STANDARD 20)

add_executable(api_tests
    api/session_test.cpp
    api/haptic_session_test.cpp
    api/c_api_test.cpp
    api/device_test.cpp
    support/fake_service.cpp
    support/win_objects.cpp)
target_include_directories(api_tests PRIVATE support ${GTEST_INCLUDE_DIR})
target_link_libraries(api_tests PRIVATE cci ccic test_protos ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY})
target_link_options(api_tests PRIVATE ${STATIC_LINK})
set_property(TARGET api_tests PROPERTY CXX_STANDARD 20)

# One CTest test per GoogleTest case, so the JUnit report lists every case.
# api_tests own the service's fixed pipe name, so they never run in parallel.
gtest_discover_tests(unit_tests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 30 LABELS unit)
gtest_discover_tests(api_tests  DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE LABELS api)
