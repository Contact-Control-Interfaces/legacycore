cmake_minimum_required(VERSION 3.20)
project(ccic CXX)

add_link_options(-static-libgcc -static-libstdc++ -static)

set(SRC_DIR src)
set(INCLUDE_DIR include)

FILE(GLOB_RECURSE SRC_LIST src/*.*)
FILE(GLOB_RECURSE INCLUDE_LIST ${INCLUDE_DIR}/*.*)

add_library(${PROJECT_NAME} SHARED ${INCLUDE_LIST} ${SRC_LIST})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "ccic")

target_link_libraries(${PROJECT_NAME} PUBLIC cci types)

target_compile_options(${PROJECT_NAME} PRIVATE -DCCIC_API_EXPORT)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "include/contactci.h")

install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${PROJECT_NAME}
        RUNTIME DESTINATION ${PROJECT_NAME}
        PUBLIC_HEADER DESTINATION ${PROJECT_NAME}/include/${PROJECT_NAME})