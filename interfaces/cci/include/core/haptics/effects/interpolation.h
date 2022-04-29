//
// Created by john_contactci on 2/24/2022.
//
#pragma once

#include <cmath>
#include <numbers>

namespace contactci::core::haptics::effects::interpolation {
    inline float ramp(float t) {
        return t;
    }

    float ease_out(float t) {
        return ::sinf(t * std::numbers::pi * 0.5f);
    }

    float ease_in(float t) {
        return 1.0f - ::cosf(t * std::numbers::pi * 0.5f);
    }

    inline static constexpr float SMOOTHING_CONSTANT = -1 * (std::numbers::pi * std::numbers::pi);

    // Logistic function normalized to positive side
    // SMOOTHING_CONSTANT must be negative, and determines the slope of the `ease_in_out` function
    // TODO rename to `smooth`?
    float ease_in_out(float t) {
        return 1 / (1 + ::expf(SMOOTHING_CONSTANT * (t - 0.5f)));
    }
}