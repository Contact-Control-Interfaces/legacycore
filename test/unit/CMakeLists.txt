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
#               and libccic.dll, talking to a fake Windows service; on Linux,
#               libcci.so and libccic.so talking to a fake daemon over a Unix
#               socket (the loopback test), run by tools/run-tests.sh
#
# On Linux there is a third program, fake_daemon, and the Python binding's
# tests (python/tests), which CTest runs with the same tools/run-tests.sh.

include(GoogleTest)

find_package(Protobuf REQUIRED)
find_path(GTEST_INCLUDE_DIR gtest/gtest.h REQUIRED)
find_library(GTEST_LIBRARY NAMES libgtest.a gtest REQUIRED)
find_library(GTEST_MAIN_LIBRARY NAMES libgtest_main.a gtest_main REQUIRED)

# The service side of the tests is per platform: named Windows objects and a
# fake service on a named pipe, or POSIX shm, futex counters and a fake daemon
# on a Unix socket. Each platform also has tests of its own.
if(WIN32)
    # Statically linked like libcci.dll, so the tests need no MinGW runtime on PATH.
    set(STATIC_LINK -static-libgcc -static-libstdc++ -static)
    set(OS_OBJECTS support/win_objects.cpp)
    set(FAKE_SERVICE support/fake_service.cpp)
    set(OS_UNIT_TESTS)
    set(OS_API_TESTS)
    set(OS_LIBRARIES)
else()
    find_package(Threads REQUIRED)
    set(STATIC_LINK)
    set(OS_OBJECTS support/posix_objects.cpp)
    set(FAKE_SERVICE support/fake_daemon_posix.cpp)
    set(OS_UNIT_TESTS internal/posix_primitives_test.cpp)
    set(OS_API_TESTS api/linux_loopback_test.cpp)
    set(OS_LIBRARIES Threads::Threads)
endif()

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
# OS primitives have one source per platform: foo_win32.cpp / foo_posix.cpp
list(FILTER CCI_SOURCES EXCLUDE REGEX "_(win32|posix)\\.cpp$")
if(WIN32)
    file(GLOB CCI_PLATFORM_SOURCES ${CMAKE_SOURCE_DIR}/cci/src/*_win32.cpp)
else()
    file(GLOB CCI_PLATFORM_SOURCES ${CMAKE_SOURCE_DIR}/cci/src/*_posix.cpp)
endif()
list(APPEND CCI_SOURCES ${CCI_PLATFORM_SOURCES})
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
    ${OS_UNIT_TESTS}
    ${OS_OBJECTS})
target_include_directories(unit_tests PRIVATE support ${GTEST_INCLUDE_DIR})
target_link_libraries(unit_tests PRIVATE cci_internal ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY} ${OS_LIBRARIES})
target_link_options(unit_tests PRIVATE ${STATIC_LINK})
set_property(TARGET unit_tests PROPERTY CXX_STANDARD 20)

add_executable(api_tests
    api/session_test.cpp
    api/haptic_session_test.cpp
    api/c_api_test.cpp
    api/device_test.cpp
    ${OS_API_TESTS}
    ${FAKE_SERVICE}
    ${OS_OBJECTS})
target_include_directories(api_tests PRIVATE support ${GTEST_INCLUDE_DIR})
target_link_libraries(api_tests PRIVATE cci ccic test_protos ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY} ${OS_LIBRARIES})
target_link_options(api_tests PRIVATE ${STATIC_LINK})
set_property(TARGET api_tests PROPERTY CXX_STANDARD 20)

# One CTest test per GoogleTest case, so the JUnit report lists every case.
# api_tests own the service's fixed pipe name, so they never run in parallel
# (on Linux each fake daemon has a socket of its own, but they stay serial too).
gtest_discover_tests(unit_tests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 30 LABELS unit)
gtest_discover_tests(api_tests  DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE LABELS api)

# The Python binding (python/contactci, INN-281) is tested where it is used, on
# Linux: its unittest modules run under CTest against the libccic.so built here.
# test_layout compiles the C headers, test_library needs only the library, and
# test_loopback drives fake_daemon, the fake daemon as a program of its own.
if(NOT WIN32)
    add_executable(fake_daemon support/fake_daemon_main.cpp ${FAKE_SERVICE} ${OS_OBJECTS})
    target_include_directories(fake_daemon PRIVATE support)
    target_link_libraries(fake_daemon PRIVATE test_protos types ${OS_LIBRARIES})
    set_property(TARGET fake_daemon PROPERTY CXX_STANDARD 20)

    # The system's python3, as a user of the binding has it: 3.10 on Ubuntu 22.04, 3.12 on 24.04
    find_program(PYTHON3_EXECUTABLE python3 REQUIRED)

    # One CTest test per module, so the JUnit report says which part failed.
    # -B: no __pycache__ in the source tree.
    foreach(module layout library loopback)
        add_test(NAME python_binding.${module}
                 COMMAND ${PYTHON3_EXECUTABLE} -B -m unittest -v tests.test_${module}
                 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/python)
        set_tests_properties(python_binding.${module} PROPERTIES
            LABELS python
            TIMEOUT 120
            RUN_SERIAL TRUE
            ENVIRONMENT "CONTACTCI_LIBRARY=$<TARGET_FILE:ccic>;CONTACTCI_TEST_FAKE_DAEMON=$<TARGET_FILE:fake_daemon>;CONTACTCI_TEST_CC=${CMAKE_C_COMPILER}")
    endforeach()
endif()
