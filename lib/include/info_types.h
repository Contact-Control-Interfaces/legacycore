//
// Created by zach_contactci on 9/11/2023.
//

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t processID;
    const char* processName;
} ClientDescription;

typedef struct {
    const char* productLine;
    const char* serialNumber;
    bool isRight;
    bool isConnected;
} DeviceDescription;

typedef struct {
    const char* version;
    bool isInteractive;
} ServiceInfo;

typedef struct {
    void *memoryHandle;
    void *leftEventHandle;
    void *rightEventHandle;
} HapticMemoryAccess;
