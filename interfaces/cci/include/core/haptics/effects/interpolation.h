//
// Created by john_contactci on 2/24/2022.
//
#pragma once

#include <cmath>

namespace contactci::core::haptics::effects::interpolation {
    inline float ramp(float t) {
        return t;
    }

    float ease_out(float t) {
        return sinf(t * M_PI * 0.5f);
    }

    float ease_in(float t) {
        return 1.0f - cosf(t * M_PI * 0.5f);
    }

    inline static constexpr float SMOOTHING_CONSTANT = -1 * (M_PI * M_PI);

    // Logistic function normalized to positive side
    // SMOOTHING_CONSTANT must be negative, and determines the slope of the `ease_in_out` function
    // TODO rename to `smooth`?
    float ease_in_out(float t) {
        return 1 / (1 + expf(SMOOTHING_CONSTANT * (t - 0.5f)));
    }
}