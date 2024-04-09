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

add_library(${PROJECT_NAME} SHARED ${INCLUDE_LIST} ${SRC_LIST} ${PROTO_SRCS} ${PROTO_HDRS})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR} PUBLIC ${PROTOBUF_INCLUDE_DIRS} ${GENERATED_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "cci")

target_link_libraries(${PROJECT_NAME} PUBLIC ${PROTOBUF_LIBRARIES} types)

target_compile_options(${PROJECT_NAME} PRIVATE -DCCI_API_EXPORT)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "public_include/contactci.h")

install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${PROJECT_NAME}
        RUNTIME DESTINATION ${PROJECT_NAME}
        PUBLIC_HEADER DESTINATION ${PROJECT_NAME}/include/${PROJECT_NAME})
