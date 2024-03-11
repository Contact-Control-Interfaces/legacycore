//
// Created by john_contactci on 2/21/2024.
//

#include <stdexcept>
#include <string>

#include "shared_memory.h"

using namespace contactci::io;

// Unnamed namespace rather than C-style static functions
// See https://stackoverflow.com/questions/154469/why-should-you-prefer-unnamed-namespaces-over-static-functions
namespace {
    std::string GetLastErrorAsString() {
        DWORD errorMessageID = GetLastError();

        if (errorMessageID == 0)
            return {};

        LPSTR messageBuffer = nullptr;

        //Ask Win32 to give us the string version of that message ID.
        //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL
        );

        std::string message(messageBuffer, size);

        LocalFree(messageBuffer);

        return message;
    }
}

SharedMemoryManager::SharedMemoryManager(const std::string& memoryName, const std::string& eventName, bool canWrite) {
    std::size_t memorySize = sizeof(HapticState) * 2;

    DWORD memAccess = FILE_MAP_READ;
    DWORD eventAccess = SYNCHRONIZE;

    if (canWrite) {
        memAccess |= FILE_MAP_WRITE;
        eventAccess |= EVENT_MODIFY_STATE;
    }

    sharedMemoryHandle = OpenFileMapping(memAccess, FALSE, memoryName.c_str());

    if (sharedMemoryHandle == NULL) {
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
    sharedMemory = MapViewOfFile(sharedMemoryHandle, memAccess, 0, 0, memorySize);

    if (sharedMemory == NULL) {
        throw std::runtime_error(
                std::string("Failed to map shared memory: MapViewOfFile; GetLastError = ")
                + GetLastErrorAsString()
        );
    }

    eventHandle = OpenEvent(eventAccess, FALSE, eventName.c_str());

    if (eventHandle == NULL) {
        throw std::runtime_error(
                std::string("Failed to open event: OpenEvent; GetLastError = ")
                + GetLastErrorAsString()
        );
    }
}

SharedMemoryManager::~SharedMemoryManager() {
    UnmapViewOfFile(sharedMemory);
    sharedMemory = nullptr;
}

HapticState *SharedMemoryManager::getLeftHapticStateMapping() {
    return static_cast<HapticState*>(sharedMemory);
}

HapticState *SharedMemoryManager::getRightHapticStateMapping() {
    return static_cast<HapticState*>(sharedMemory) + 1;
}

void SharedMemoryManager::signalEvent() {
    // Set and immediately unset event; this will wake all threads waiting on `eventHandle` and then they'll
    // Wait again after processing shared mem
    if (!SetEvent(eventHandle)) {
        throw std::runtime_error(
                std::string("Failed to signal event: SetEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }

    if (!ResetEvent(eventHandle)) {
        throw std::runtime_error(
                std::string("Failed to reset event: ResetEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }
}