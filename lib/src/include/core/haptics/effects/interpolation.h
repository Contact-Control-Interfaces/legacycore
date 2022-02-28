//
// Created by john_contactci on 2/24/2022.
//
#pragma once

#include <cmath>

namespace contactci::core::haptics::effects::interpolation {

    const auto&& ramp = [](float t){ return t; };
    const auto&& ease_out = [](float t){return ::sin(t * std::numbers::pi * 0.5f); };
    const auto&& ease_in = [](float t){return 1.0f - ::cos(t * std::numbers::pi * 0.5f); };

    // Logistic function normalized to positive side
    // SMOOTHING_CONSTANT must be negative, and determines the slope of the `ease_in_out` function
    // TODO rename to `smooth`?
    static constexpr float SMOOTHING_CONSTANT = -1 * (std::numbers::pi * std::numbers::pi);
    const auto&& ease_in_out = [](float t){ return 1 / (1 + ::expf(SMOOTHING_CONSTANT * (t - 0.5f))); };
}

