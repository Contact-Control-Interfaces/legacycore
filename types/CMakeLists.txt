cmake_minimum_required(VERSION 3.20)
project(types C)

set(INCLUDE_DIR include)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE ${INCLUDE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "include/ccic/lib_defs.h;include/ccic/haptic_state.h;include/ccic/error.h")

install(TARGETS ${PROJECT_NAME}
        PUBLIC_HEADER DESTINATION cci/include/cci)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "include/ccic/lib_defs.h;include/ccic/haptic_state.h;include/ccic/error.h")

install(TARGETS ${PROJECT_NAME}
        PUBLIC_HEADER DESTINATION ccic/include/ccic)