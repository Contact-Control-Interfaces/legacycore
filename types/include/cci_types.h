//
// Created by zach_contactci on 9/11/2023.
//

#pragma once

#include <stdint.h>
#include <stdbool.h>

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

typedef struct ServiceInfo {
    const char* version;
    bool isInteractive;
} ServiceInfo;

typedef struct HapticState {
    float thumbVibrationAmplitude;
    float indexVibrationAmplitude;
    float middleVibrationAmplitude;
    float ringVibrationAmplitude;
    float littleVibrationAmplitude;

    uint8_t thumbMotorAmplitude;
    uint8_t indexMotorAmplitude;
    uint8_t middleMotorAmplitude;
    uint8_t ringMotorAmplitude;
    uint8_t littleMotorAmplitude;

    float thumbMotorPosition;
    float indexMotorPosition;
    float middleMotorPosition;
    float ringMotorPosition;
    float littleMotorPosition;

    uint8_t thumbVibrationEffect;
    uint8_t indexVibrationEffect;
    uint8_t middleVibrationEffect;
    uint8_t ringVibrationEffect;
    uint8_t littleVibrationEffect;

    uint8_t thumbVibrationModifier;
    uint8_t indexVibrationModifier;
    uint8_t middleVibrationModifier;
    uint8_t ringVibrationModifier;
    uint8_t littleVibrationModifier;
} HapticState;