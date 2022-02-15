//
// Created by john_contactci on 2/3/2022.
//

#include "core/haptics/effects/haptic_effect.h"

#include <functional>

using namespace contactci::core::haptics;

effects::HapticEffect::HapticEffect(uint32_t duration, double scalar) : duration(duration), scalar(scalar) { }

uint32_t effects::HapticEffect::HapticEffect::get_duration() const {
    return duration;
}

double effects::HapticEffect::HapticEffect::get_scalar() const {
    return scalar;
}

effects::ZeroHapticEffect::ZeroHapticEffect() : ZeroHapticEffect(0) { }
effects::ZeroHapticEffect::ZeroHapticEffect(const ZeroHapticEffect &other) : HapticEffect(other.duration, other.scalar) { }
effects::ZeroHapticEffect::ZeroHapticEffect(uint32_t duration) : HapticEffect(duration) { }

Frame &effects::ZeroHapticEffect::get_current_frame() {
    return null_frame;
}

void effects::ZeroHapticEffect::move_next_frame() { }

effects::HapticEffectSequence effects::ZeroHapticEffect::chain(HapticEffect &other) {
    return HapticEffectSequence(*this).chain(other);
}

// TODO scale(0) "deletes" it
effects::ZeroHapticEffect effects::ZeroHapticEffect::scale(double scalar) {
    ZeroHapticEffect new_zero(*this);

    new_zero.scalar = scalar;

    return new_zero;
}

contactci::core::haptics::effects::ZeroHapticEffect zero_effect;
//TODO no idea why this cast is needed
contactci::core::haptics::effects::ZeroHapticEffect &contactci::core::haptics::effects::HapticEffect::ZERO = zero_effect;

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
