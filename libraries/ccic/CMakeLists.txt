cmake_minimum_required(VERSION 3.20)
project(library_ccic)

set(SRC_DIR src)
set(INCLUDE_DIR include)

FILE(GLOB_RECURSE SRC_LIST src/*.*)
FILE(GLOB_RECURSE INCLUDE_LIST ${INCLUDE_DIR}/*.*)

add_library(${PROJECT_NAME} SHARED ${INCLUDE_LIST} ${SRC_LIST})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "ccic")

#TODO this will have to depend on all comms lib implementations; probably not ideal
target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        -static
            interface_library_core
            library_io_pipe
)

target_compile_options(${PROJECT_NAME} PRIVATE -DCCI_API_EXPORT)