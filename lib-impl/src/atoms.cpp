//
// Created by john_contactci on 3/9/2022.
//

#include "../include/atoms.h"

#include <core/haptics/effects/player.h>

#include <algorithm>

using namespace contactci::core::haptics::atoms;

template <typename T>
static inline T clamp(T value, T min, T max) {
    return std::min(max, std::max(min, value));
}

VibrationAtom::VibrationAtom(uint8_t effect) : effect(effect) { }

bool VibrationAtom::equals(VibrationAtom other) const {
    return effect == other.effect;
}

VibrationAtom VibrationAtom::lerp(VibrationAtom to, double t) const {
    const uint8_t diff = to.effect - this->effect;
    return VibrationAtom(this->effect + (diff * t));
}

uint8_t VibrationAtom::get_effect() {
    return effect;
}

VibrationAtom VibrationAtom::ZERO(0);

template<>
VibrationAtom Atom<VibrationAtom>::get_zero() {
    return VibrationAtom::ZERO;
}

template<>
void AtomPlayer<VibrationAtom>::play(contactci::comms::Communicator &comms, VibrationAtom atom) {
    comms.send(atom.get_effect());
}

ForceFeedbackAtom::ForceFeedbackAtom(float amplitude) : amplitude(clamp(amplitude, 0.0f, 1.0f)) { }

ForceFeedbackAtom ForceFeedbackAtom::lerp(ForceFeedbackAtom to, double t) const {
    const float diff = to.amplitude - this->amplitude;
    return ForceFeedbackAtom(this->amplitude + (diff * t));
}

bool ForceFeedbackAtom::equals(ForceFeedbackAtom other) const {
    return amplitude == other.amplitude;
}

float ForceFeedbackAtom::get_amplitude() {
    return amplitude;
}

ForceFeedbackAtom ForceFeedbackAtom::ZERO(0);

template<>
ForceFeedbackAtom Atom<ForceFeedbackAtom>::get_zero() {
    return ForceFeedbackAtom::ZERO;
}

template<>
void AtomPlayer<ForceFeedbackAtom>::play(contactci::comms::Communicator &comms, ForceFeedbackAtom atom) {
    comms.send(atom.get_amplitude() * 255);
}