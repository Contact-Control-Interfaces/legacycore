//
// Created by zach_contactci on 9/11/2023.
//

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "haptic_state.h"

typedef struct ClientDescription {
    uint32_t processID;
    const char* processName;
} ClientDescription;

typedef struct DeviceDescription {
    const char* productLine;
    const char* serialNumber;
    bool isRight;
    bool isConnected;
} DeviceDescription;
