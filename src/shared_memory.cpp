//
// Created by john_contactci on 2/21/2024.
//

#include <stdexcept>

#include "shared_memory.h"

using namespace contactci::io;

SharedMemoryManager::SharedMemoryManager(bool isRight) {
    LPCSTR memoryName = TEXT(isRight ? "Global\\contactci-shm-right" : "Global\\contactci-shm-left");
    LPCSTR eventName = TEXT(isRight ? "Global\\contactci-event-right" : "Global\\contactci-event-left");

    sharedMemoryHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, memoryName);

    if (sharedMemoryHandle == NULL) {
        throw std::runtime_error(
                std::string("Failed to open shared memory: OpenFileMapping; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }

    sharedMemory = MapViewOfFile(sharedMemoryHandle, FILE_MAP_WRITE, 0, 0, sizeof(HapticState));

    if (sharedMemory == NULL) {
        throw std::runtime_error(
                std::string("Failed to map shared memory: MapViewOfFile; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }

    eventHandle = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventName);

    if (eventHandle == NULL) {
        throw std::runtime_error(
                std::string("Failed to open event: OpenEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }
}

SharedMemoryManager::~SharedMemoryManager() {
    UnmapViewOfFile(sharedMemory);
    CloseHandle(sharedMemoryHandle);
    CloseHandle(eventHandle);
}

HapticState *SharedMemoryManager::getHapticStateMapping() {
    return static_cast<HapticState*>(sharedMemory);
}

void SharedMemoryManager::signalEvent() {
    if (!SetEvent(eventHandle)) {
        throw std::runtime_error(
                std::string("Failed to signal event: SetEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }
}
