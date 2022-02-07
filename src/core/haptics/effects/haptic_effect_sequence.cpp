//
// Created by john_contactci on 2/3/2022.
//

#include "core/haptics/effects/haptic_effect_sequence.h"

#include <functional>

using namespace contactci::core::haptics;

effects::HapticEffectSequence::HapticEffectSequence(HapticEffect &initial)
        : TypedHapticEffect<HapticEffectSequence>(),
          sequence(std::vector<std::reference_wrapper<HapticEffect>> { std::ref(initial) }),
          current_effect(initial)
{ }

effects::HapticEffectSequence::HapticEffectSequence(const HapticEffectSequence &other)
        : TypedHapticEffect<HapticEffectSequence>(other),
          sequence(other.sequence),
          current_effect(sequence.front())
{ }

effects::HapticEffectSequence effects::HapticEffectSequence::scale(double scalar) {
    auto copy = HapticEffectSequence(*this);

    copy.scalar = scalar;

    return copy;
}

effects::HapticEffectSequence effects::HapticEffectSequence::chain(HapticEffect &other) {
    auto copy = HapticEffectSequence(*this);

    copy.sequence.emplace_back(other);

    return copy;
}

Frame &effects::HapticEffectSequence::get_current_frame() {
    return current_effect.get_current_frame();
}

void effects::HapticEffectSequence::move_next_frame() {
    // TODO
}

uint32_t effects::HapticEffectSequence::get_duration() const {
    uint32_t sum = 0;

    for(auto it = sequence.begin(); it != sequence.end(); it = std::next(it)) {
        sum += it->get().get_duration();
    }

    return sum;
}
