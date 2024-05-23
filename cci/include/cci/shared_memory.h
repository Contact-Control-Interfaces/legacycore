//
// Created by john_contactci on 2/21/2024.
//
#pragma once

#include <string>
#include <windows.h>

namespace contactci {
    class SharedMemoryManager {
    public:
        SharedMemoryManager(const std::string& memoryName, std::size_t size, bool canWrite);
        ~SharedMemoryManager();

        LPVOID get_memory();

    private:
        HANDLE sharedMemoryHandle;
        LPVOID sharedMemory;
    };
}

