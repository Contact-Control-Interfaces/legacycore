cmake_minimum_required(VERSION 3.20)
project(library_cci)

set(CMAKE_CXX_STANDARD 20)

set(SRC_DIR src)
set(INCLUDE_DIR include)

FILE(GLOB_RECURSE SRC_LIST src/*.*)
FILE(GLOB_RECURSE INCLUDE_LIST ${INCLUDE_DIR}/*.*)

add_library(${PROJECT_NAME} SHARED ${INCLUDE_LIST} ${SRC_LIST})

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "cci")

target_link_libraries(${PROJECT_NAME} PUBLIC interface_library_core interface_cci)

target_compile_options(${PROJECT_NAME} PRIVATE -DCCI_API_EXPORT)