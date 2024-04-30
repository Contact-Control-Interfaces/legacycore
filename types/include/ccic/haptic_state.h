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
    float thumbForceFeedbackAmplitude;
    float indexForceFeedbackAmplitude;
    float middleForceFeedbackAmplitude;
    float ringForceFeedbackAmplitude;
    float littleForceFeedbackAmplitude;

    float thumbForceFeedbackPosition;
    float indexForceFeedbackPosition;
    float middleForceFeedbackPosition;
    float ringForceFeedbackPosition;
    float littleForceFeedbackPosition;

    float thumbVibrationAmplitude;
    float indexVibrationAmplitude;
    float middleVibrationAmplitude;
    float ringVibrationAmplitude;
    float littleVibrationAmplitude;

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
