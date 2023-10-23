//
// Created by zach_contactci on 9/11/2023.
//

#ifndef CONTACTCI_INFO_TYPES_H
#define CONTACTCI_INFO_TYPES_H

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

#endif //CONTACTCI_INFO_TYPES_H
