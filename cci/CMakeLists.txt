cmake_minimum_required(VERSION 3.20)
project(cci CXX)

add_link_options(-static-libgcc -static-libstdc++ -static)

set(SRC_DIR src)
set(INCLUDE_DIR include)

FILE(GLOB_RECURSE SRC_LIST src/*.*)
FILE(GLOB_RECURSE INCLUDE_LIST ${INCLUDE_DIR}/*.*)
FILE(GLOB_RECURSE PROTOS_LIST packets/proto/*.proto)

find_package(Protobuf REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS_LIST})

# Get the directory for the generated protobuf files
list(GET PROTO_HDRS 0 FIRST_HEADER)
get_filename_component(GENERATED_DIR ${FIRST_HEADER} DIRECTORY)

add_library(${PROJECT_NAME}_object OBJECT ${INCLUDE_LIST} ${SRC_LIST} ${PROTO_SRCS} ${PROTO_HDRS})
set_target_properties(${PROJECT_NAME}_object PROPERTIES POSITION_INDEPENDENT_CODE 1)

add_library(${PROJECT_NAME}_shared SHARED $<TARGET_OBJECTS:cci_object>)
add_library(${PROJECT_NAME}_static STATIC $<TARGET_OBJECTS:cci_object>)

set_property(TARGET ${PROJECT_NAME}_object PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME}_shared PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME}_static PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME}_object PUBLIC ${INCLUDE_DIR} PUBLIC ${PROTOBUF_INCLUDE_DIRS} ${GENERATED_DIR})
target_include_directories(${PROJECT_NAME}_shared PUBLIC ${INCLUDE_DIR} PUBLIC ${PROTOBUF_INCLUDE_DIRS} ${GENERATED_DIR})
target_include_directories(${PROJECT_NAME}_static PUBLIC ${INCLUDE_DIR} PUBLIC ${PROTOBUF_INCLUDE_DIRS} ${GENERATED_DIR})

set_target_properties(${PROJECT_NAME}_object PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME}_shared PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME}_static PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(${PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME "cci")
set_target_properties(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME "cci")

target_link_libraries(${PROJECT_NAME}_object PUBLIC ${PROTOBUF_LIBRARIES} types)
target_link_libraries(${PROJECT_NAME}_shared PUBLIC ${PROTOBUF_LIBRARIES} types)
target_link_libraries(${PROJECT_NAME}_static PUBLIC ${PROTOBUF_LIBRARIES} types)

target_compile_options(${PROJECT_NAME}_object PRIVATE -DCCI_API_EXPORT)
target_compile_options(${PROJECT_NAME}_shared PRIVATE -DCCI_API_EXPORT)
target_compile_options(${PROJECT_NAME}_static PRIVATE -DCCI_API_EXPORT)