cmake_minimum_required(VERSION 3.20)
project(library_ccic)

#set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
#set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_link_options(-static-libgcc -static-libstdc++ -static)

set(SRC_DIR src)
set(INCLUDE_DIR include)
set(PRIVATE_INCLUDE_DIR src/include)

FILE(GLOB_RECURSE SRC_LIST src/*.*)
FILE(GLOB_RECURSE INCLUDE_LIST ${INCLUDE_DIR}/*.*)
FILE(GLOB_RECURSE PROTOS_LIST packets/proto/*.proto)

find_package(Protobuf REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS_LIST})

# Get the directory for the generated protobuf files
list(GET PROTO_HDRS 0 FIRST_HEADER)
get_filename_component(GENERATED_DIR ${FIRST_HEADER} DIRECTORY)

add_library(${PROJECT_NAME} SHARED ${INCLUDE_LIST} ${SRC_LIST} ${PROTO_SRCS} ${PROTO_HDRS})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR} PRIVATE ${PRIVATE_INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIRS} ${GENERATED_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "ccic")

target_link_libraries(${PROJECT_NAME} PUBLIC ${PROTOBUF_LIBRARIES})

target_compile_options(${PROJECT_NAME} PRIVATE -DCCI_API_EXPORT)