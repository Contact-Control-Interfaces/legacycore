//
// Created by john_contactci on 2/1/2022.
//

#pragma once

#include <cstdint>

#include "core/haptics/frame.h"

namespace contactci::core::haptics::effects {

    class HapticEffectSequence;

    class HapticEffect {
    public:
        virtual Frame &get_current_frame() = 0;
        virtual void move_next_frame() = 0;
        virtual uint32_t get_duration() = 0;

        virtual HapticEffectSequence chain(HapticEffect &other) = 0;

        ~HapticEffect() = default;

    protected:
        explicit HapticEffect(double scalar = 1.0);
        double scalar;
    };

    // Can't use concept constraints with CRTP due to incomplete types; use static_assert in base constructor instead
    template <typename T>
    class TypedHapticEffect : public HapticEffect {
    public:
        virtual T scale(double scalar) = 0;
    protected:
        explicit TypedHapticEffect(double scalar = 1.0);
    };
}

template <typename T>
contactci::core::haptics::effects::TypedHapticEffect<T>::TypedHapticEffect(double scalar)
        : contactci::core::haptics::effects::HapticEffect(scalar) {
    static_assert(std::is_base_of<HapticEffect, T>::value);
}
