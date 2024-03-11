//
// Created by john_contactci on 2/21/2024.
//
#pragma once

#include "haptics.h"

#include <string>
#include <windows.h>

namespace contactci::io {
    class SharedMemoryManager {
    public:
        SharedMemoryManager(const std::string& memoryName, const std::string& eventName, bool canWrite);
        ~SharedMemoryManager();

        HapticState *getLeftHapticStateMapping();
        HapticState *getRightHapticStateMapping();

        void signalEvent();

    private:
        HANDLE sharedMemoryHandle;
        LPVOID sharedMemory;
        HANDLE eventHandle;
    };
}

