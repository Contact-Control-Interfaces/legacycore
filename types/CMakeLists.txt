cmake_minimum_required(VERSION 3.20)
project(types C)

set(INCLUDE_DIR include)

add_library(${PROJECT_NAME} INTERFACE)

# ccic/haptic_state.h includes <contactci/haptic_state.h>, the shared-memory
# contract types from the communication submodule (DEV-16). cci and ccic link
# this target PUBLIC, so both get the submodule's include/ too.
set(SHARED_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cci/packets/include)
if(NOT EXISTS ${SHARED_INCLUDE_DIR}/contactci/haptic_state.h)
    message(FATAL_ERROR "cci/packets/include/contactci/haptic_state.h is missing. Run: git submodule update --init --recursive")
endif()
target_include_directories(${PROJECT_NAME} INTERFACE ${INCLUDE_DIR} ${SHARED_INCLUDE_DIR})

# Every install tree that has ccic/haptic_state.h ships contactci/haptic_state.h
# next to it, so the installed header compiles.
if(WIN32)
    install(FILES ${SHARED_INCLUDE_DIR}/contactci/haptic_state.h DESTINATION cci/include/contactci)
    install(FILES ${SHARED_INCLUDE_DIR}/contactci/haptic_state.h DESTINATION ccic/include/contactci)
endif()
if(UNIX)
    include(GNUInstallDirs)
    install(FILES ${SHARED_INCLUDE_DIR}/contactci/haptic_state.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/contactci)
endif()

# Windows deploy layout. Linux installs from the root CMakeLists.txt.
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "include/ccic/lib_defs.h;include/ccic/haptic_state.h;include/ccic/error.h")

    install(TARGETS ${PROJECT_NAME}
            PUBLIC_HEADER DESTINATION cci/include/cci)

    set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "include/ccic/lib_defs.h;include/ccic/haptic_state.h;include/ccic/error.h")

    install(TARGETS ${PROJECT_NAME}
            PUBLIC_HEADER DESTINATION ccic/include/ccic)
endif()