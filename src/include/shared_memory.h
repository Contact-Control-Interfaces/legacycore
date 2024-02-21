//
// Created by john_contactci on 2/21/2024.
//
#pragma once

#include <cstdint>

#include <windows.h>

namespace contactci::io {
    struct HapticState {
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
    };

    class SharedMemoryManager {
    public:
        explicit SharedMemoryManager(bool isRight);
        ~SharedMemoryManager();

        HapticState *getHapticStateMapping();
        void signalEvent();

    private:
        HANDLE sharedMemoryHandle;
        LPVOID sharedMemory;
        HANDLE eventHandle;
    };
}

