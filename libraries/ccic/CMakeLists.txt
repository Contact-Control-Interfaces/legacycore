cmake_minimum_required(VERSION 3.20)
project(library_ccic)

set(CMAKE_CXX_STANDARD 20)

set(SRC_DIR src)
set(INCLUDE_DIR include)

FILE(GLOB_RECURSE SRC_LIST src/*.*)
FILE(GLOB_RECURSE INCLUDE_LIST ${INCLUDE_DIR}/*.*)

add_library(${PROJECT_NAME} SHARED ${INCLUDE_LIST} ${SRC_LIST})

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "ccic")

#TODO this will have to depend on all comms lib implementations; probably not ideal
target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        -static
            interface_library_core
            library_cci
        -dynamic
            library_comms_stdout
            library_comms_unity_console
)

target_compile_options(${PROJECT_NAME} PRIVATE -DCCI_API_EXPORT)