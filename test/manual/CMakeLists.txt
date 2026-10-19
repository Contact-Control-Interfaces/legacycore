cmake_minimum_required(VERSION 3.20)
project(test)

add_subdirectory(cci)
add_subdirectory(ccic)

# The device tools use windows.h
if(WIN32)
    add_subdirectory(cci_devices)
    add_subdirectory(ccic_devices)
endif()
