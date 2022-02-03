//
// Created by john_contactci on 2/3/2022.
//

#pragma once

#include "core/haptics/effects/haptic_effect.h"

#include <vector>

namespace contactci::core::haptics::effects {

    class HapticEffectSequence : public TypedHapticEffect<HapticEffectSequence> {
    public:
        explicit HapticEffectSequence(HapticEffect &initial);

        HapticEffectSequence(const HapticEffectSequence &other);

        HapticEffectSequence scale(double scalar) override;

        HapticEffectSequence chain(HapticEffect &other) override;

        Frame &get_current_frame() override;

        void move_next_frame() override;

        uint32_t get_duration() override;

    protected:
        std::vector<std::reference_wrapper<HapticEffect>> sequence;
        HapticEffect &current_effect;
    };
}