cmake_minimum_required(VERSION 3.25)
project(library_packet)

set(INCLUDE_DIR include)

FILE(GLOB_RECURSE INCLUDE_LIST ${INCLUDE_DIR}/*.*)
FILE(GLOB_RECURSE PROTOS_LIST shared/proto/*.proto)

find_package(Protobuf REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS EXPORT_MACRO CCI_API ${PROTOS_LIST})

# Get the directory for the generated protobuf files
list(GET PROTO_HDRS 0 FIRST_HEADER)
get_filename_component(GENERATED_DIR ${FIRST_HEADER} DIRECTORY)

include_directories(${GENERATED_DIR})

add_library(${PROJECT_NAME} SHARED ${INCLUDE_LIST} ${PROTO_SRCS} ${PROTO_HDRS})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIRS} ${GENERATED_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "packet")

target_link_libraries(${PROJECT_NAME} PUBLIC interface_library_core ${PROTOBUF_LIBRARIES})

include(CheckCCompilerFlag)
check_c_compiler_flag("-FI ${interface_library_core_SOURCE_DIR}/include/cci_lib_defs.h" HAS_FI_FLAG)

if (HAS_FI_FLAG)
    target_compile_options(${PROJECT_NAME} PRIVATE -FI ${interface_library_core_SOURCE_DIR}/include/cci_lib_defs.h)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -include ${interface_library_core_SOURCE_DIR}/include/cci_lib_defs.h)
endif()

target_compile_options(${PROJECT_NAME} PRIVATE -DCCI_API_EXPORT)
