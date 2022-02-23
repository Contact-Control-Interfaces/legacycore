//
// Created by john_contactci on 2/3/2022.
//

#include <core/haptics/effects/haptic_effect.h>
#include <core/haptics/effects/player.h>

#include <comms/communicator.h>

#include <iostream>
#include <list>

using namespace contactci::core::haptics;
using namespace contactci::core::haptics::effects;

class VibrationDimension;
class ForceFeedbackDimension;
class PressureDimension;

class VibrationAtom : public Atom<VibrationAtom> {
public:
    static VibrationAtom ZERO;
    explicit VibrationAtom(uint8_t effect) : effect(effect) { }

    template <typename T>
    bool operator==(const T &other) const {
        return false;
    }

    uint8_t get_effect() {
        return effect;
    }

private:
    uint8_t effect;
};

template <>
bool VibrationAtom::operator==<VibrationAtom>(const VibrationAtom &another) const {
    return effect == another.effect;
}

VibrationAtom VibrationAtom::ZERO(0);

class ForceFeedbackAtom : public Atom<ForceFeedbackAtom> {
public:
    static ForceFeedbackAtom ZERO;

    explicit ForceFeedbackAtom(float amplitude) : amplitude(amplitude) { }

    template <typename T>
    bool operator==(const T &other) const {
        return false;
    }

    float get_amplitude() {
        return amplitude;
    }

private:
    float amplitude; // 0 - 1
};

template <>
bool ForceFeedbackAtom::operator==<ForceFeedbackAtom>(const ForceFeedbackAtom &another) const {
    return amplitude == another.amplitude;
}

ForceFeedbackAtom ForceFeedbackAtom::ZERO(0);

class PressureAtom : public Atom<PressureAtom> {
public:
    static PressureAtom ZERO;

    explicit PressureAtom(float amplitude) : amplitude(amplitude) { }

    template <typename T>
    bool operator==(const T &other) const {
        return false;
    }

    float get_amplitude() {
        return amplitude;
    }

private:
    float amplitude; // 0 - 1
};

template <>
bool PressureAtom::operator==<PressureAtom>(const PressureAtom &another) const {
    return amplitude == another.amplitude;
}

PressureAtom PressureAtom::ZERO(0);

class DebugCommunicator : public contactci::comms::Communicator {
public:
    void start_frame() override {
        /* TODO */
    }

    void end_frame() override {
        std::cout << std::endl;
    }

    void send(uint8_t byte) override {
        std::cout << byte;
    }
};

template<>
void AtomPlayer<VibrationAtom>::play(contactci::comms::Communicator &comms, VibrationAtom &atom) {
    comms.send(atom == VibrationAtom::ZERO ? '0' : '~');
}

template<>
void AtomPlayer<ForceFeedbackAtom>::play(contactci::comms::Communicator &comms, ForceFeedbackAtom &atom) {
    comms.send(atom == ForceFeedbackAtom::ZERO ? '0' : '#');
}

template<>
void AtomPlayer<PressureAtom>::play(contactci::comms::Communicator &comms, PressureAtom &atom) {
    comms.send(atom == PressureAtom::ZERO ? '0' : '|');
}

template<>
VibrationAtom &Atom<VibrationAtom>::get_zero() {
    return VibrationAtom::ZERO;
}

template<>
ForceFeedbackAtom &Atom<ForceFeedbackAtom>::get_zero() {
    return ForceFeedbackAtom::ZERO;
}

template<>
PressureAtom &Atom<PressureAtom>::get_zero() {
    return PressureAtom::ZERO;
}

int main() {
    auto vs = VibrationAtom(52);
    auto fs = ForceFeedbackAtom(175);
    auto ps = PressureAtom(0);

    HapticEffect<VibrationAtom, ForceFeedbackAtom, PressureAtom> temp3(vs, fs, ps);

    DebugCommunicator comms;

    // ramp
    // curves / splines
    // fade = ramp/curve toward zero
    // windowed diffuse

    auto temp4 = temp3.sleep(5);

    EffectPlayer::play(comms, temp4);

    return 0;
}