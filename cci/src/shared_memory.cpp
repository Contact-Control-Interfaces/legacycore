//
// Created by john_contactci on 2/21/2024.
//

#include <stdexcept>
#include <string>

#include "shared_memory.h"
#include "winerr_util.h"

using namespace contactci;

SharedMemoryManager::SharedMemoryManager(const std::string& memoryName, std::size_t size, bool canWrite) {
    DWORD memAccess = FILE_MAP_READ;

    if (canWrite)
        memAccess |= FILE_MAP_WRITE;

    sharedMemoryHandle = OpenFileMapping(memAccess, FALSE, memoryName.c_str());

    if (sharedMemoryHandle == nullptr) {
        throw std::runtime_error(
                std::string("Failed to open shared memory: OpenFileMapping; GetLastError = ")
                + GetLastErrorAsString()
        );
    }

    /*
     * It would be nice to create two separate views with one being offset by sizeof(HapticState)
     * in order to have isolated views of the left vs right HapticState. However, the offset passed to
     * MapViewOfFile needs to be a multiple of the "allocation granularity" (which is like 64kb), so instead
     * we just have to manually offset the pointer by sizeof(HapticState)
     */
    sharedMemory = MapViewOfFile(sharedMemoryHandle, memAccess, 0, 0, size);

    if (sharedMemory == nullptr) {
        throw std::runtime_error(
                std::string("Failed to map shared memory: MapViewOfFile; GetLastError = ")
                + GetLastErrorAsString()
        );
    }
}

SharedMemoryManager::~SharedMemoryManager() {
    UnmapViewOfFile(sharedMemory);
    sharedMemory = nullptr;
}

LPVOID SharedMemoryManager::get_memory() {
    return sharedMemory;
}