cmake_minimum_required(VERSION 3.20)
project(contactci_lib)

set(CMAKE_CXX_STANDARD 20)

cmake_path(SET MAESTRO_LIBS_DIR "$ENV{LOCALAPPDATA}/Contact Control Interfaces/libs/")

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src/)
set(INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/include/)

FILE(GLOB_RECURSE SRC_LIST ${CMAKE_CURRENT_LIST_DIR}/src/*.*)
FILE(GLOB_RECURSE INCLUDE_LIST ${INCLUDE_DIR}/*.*)

include_directories(${INCLUDE_DIR})

add_library(${PROJECT_NAME} SHARED ${INCLUDE_LIST} ${SRC_LIST})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "contactci")

# Installation
cmake_path(SET CMAKE_INSTALL_PREFIX "${MAESTRO_LIBS_DIR}/${PROJECT_NAME}")

#TODO make clean work

install (TARGETS ${PROJECT_NAME} CONFIGURATIONS Debug DESTINATION Debug/lib)
install (TARGETS ${PROJECT_NAME} CONFIGURATIONS Release DESTINATION lib)

install (DIRECTORY ${INCLUDE_DIR} CONFIGURATIONS Debug DESTINATION Debug/include)
install (DIRECTORY ${INCLUDE_DIR} CONFIGURATIONS Release DESTINATION include)

# Resulting binary should be libcontactci.dll