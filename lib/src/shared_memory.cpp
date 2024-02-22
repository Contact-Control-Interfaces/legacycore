//
// Created by john_contactci on 2/21/2024.
//

#include <stdexcept>
#include <string>

#include "shared_memory.h"

#include <winternl.h>

using namespace contactci::io;

std::string GetLastErrorAsString()
{
    //Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0) {
        return std::string(); //No error message has been recorded
    }

    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);

    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);

    return message;
}

SharedMemoryManager::SharedMemoryManager(bool isRight) {
    LPCSTR memoryName = TEXT(isRight ? "Global\\contactci-shm-right" : "Global\\contactci-shm-left");
    LPCSTR eventName = TEXT(isRight ? "Global\\contactci-event-right" : "Global\\contactci-event-left");

    sharedMemoryHandle = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, memoryName);

    if (sharedMemoryHandle == NULL) {
        throw std::runtime_error(
                std::string("Failed to open shared memory: OpenFileMapping; GetLastError = ")
                + GetLastErrorAsString()
        );
    }

    sharedMemory = MapViewOfFile(sharedMemoryHandle, FILE_MAP_WRITE, 0, 0, sizeof(HapticState));

    if (sharedMemory == NULL) {
        throw std::runtime_error(
                std::string("Failed to map shared memory: MapViewOfFile; GetLastError = ")
                + GetLastErrorAsString()
        );
    }

    eventHandle = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, eventName);

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

    if (!ResetEvent(eventHandle)) {
        throw std::runtime_error(
                std::string("Failed to reset event: ResetEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }
}
