//
// Created by john_contactci on 2/1/2022.
//

#include "core/haptics/haptic_effect.h"
#include <functional>

using namespace contactci::core::haptics;

HapticEffectSequence::HapticEffectSequence(const HapticEffectSequence &other) : TypedHapticEffect<HapticEffectSequence>(other) {
    this->scalar = other.scalar;
    std::copy(other.sequence.begin(), other.sequence.end(), this->sequence.begin());
}

HapticEffectSequence HapticEffectSequence::scale(double scalar) {
    auto copy = HapticEffectSequence(*this);

    copy.scalar = scalar;

    return copy;
}

HapticEffectSequence HapticEffectSequence::chain(pointer other) {
    auto copy = HapticEffectSequence(*this);

    copy.sequence.emplace_back(other);

    return copy;
}

Frame HapticEffectSequence::get_current_frame() {
    return {};
}

void HapticEffectSequence::move_next_frame() {
    // TODO
}

uint32_t HapticEffectSequence::get_duration() {
    uint32_t sum = 0;

    for(auto it = sequence.begin(); it != sequence.end(); it = std::next(it)) {
        sum += it->get().get_duration();
    }

    return sum;
}

HapticEffect::HapticEffect(double scale)
    : scalar(scale) { }