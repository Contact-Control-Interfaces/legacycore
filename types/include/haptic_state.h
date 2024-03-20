//
// Created by john_contactci on 3/20/2024.
//

#pragma once

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

typedef struct HapticState {
    float thumbVibrationAmplitude;
    float indexVibrationAmplitude;
    float middleVibrationAmplitude;
    float ringVibrationAmplitude;
    float littleVibrationAmplitude;

    float thumbMotorAmplitude;
    float indexMotorAmplitude;
    float middleMotorAmplitude;
    float ringMotorAmplitude;
    float littleMotorAmplitude;

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
