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

class VibrationAtom : public Atom<VibrationAtom> {
public:
    static VibrationAtom ZERO;
    explicit VibrationAtom(uint8_t effect) : effect(effect) { }

    VibrationAtom add(VibrationAtom b) const override {
        return VibrationAtom(effect + b.effect);
    }

    VibrationAtom subtract(VibrationAtom b) const override {
        return VibrationAtom(effect - b.effect);
    }

    VibrationAtom scale(double scalar) const override {
        return VibrationAtom(effect * scalar);
    }

    bool equals(VibrationAtom other) const override {
        return effect == other.effect;
    }

    uint8_t get_effect() {
        return effect;
    }

private:
    uint8_t effect;
};

VibrationAtom VibrationAtom::ZERO(0);

template <typename T>
T clamp(T value, T min, T max) {
    return std::min(max, std::max(min, value));
}

class ForceFeedbackAtom : public Atom<ForceFeedbackAtom> {
public:
    static ForceFeedbackAtom ZERO;

    explicit ForceFeedbackAtom(float amplitude) : amplitude(clamp(amplitude, 0.0f, 1.0f)) { }

    ForceFeedbackAtom add(ForceFeedbackAtom b) const override {
        return ForceFeedbackAtom(amplitude + b.amplitude);
    }

    ForceFeedbackAtom subtract(ForceFeedbackAtom b) const override {
        return ForceFeedbackAtom(amplitude - b.amplitude);
    }

    ForceFeedbackAtom scale(double scalar) const override {
        return ForceFeedbackAtom(amplitude * scalar);
    }

    bool equals(ForceFeedbackAtom other) const override {
        return amplitude == other.amplitude;
    }

    float get_amplitude() {
        return amplitude;
    }

private:
    float amplitude; // 0 - 1
};

ForceFeedbackAtom ForceFeedbackAtom::ZERO(0);

class PressureAtom : public Atom<PressureAtom> {
public:
    static PressureAtom ZERO;

    explicit PressureAtom(float amplitude) : amplitude(clamp(amplitude, 0.0f, 1.0f)) { }

    PressureAtom add(PressureAtom b) const override {
        return PressureAtom(amplitude + b.amplitude);
    }

    PressureAtom subtract(PressureAtom b) const override {
        return PressureAtom(amplitude - b.amplitude);
    }

    PressureAtom scale(double scalar) const override {
        return PressureAtom(amplitude * scalar);
    }

    bool equals(PressureAtom other) const override {
        return amplitude == other.amplitude;
    }

    float get_amplitude() {
        return amplitude;
    }

private:
    float amplitude; // 0 - 1
};

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
        std::cout << (int)byte << '\t';
    }
};

template<>
void AtomPlayer<VibrationAtom>::play(contactci::comms::Communicator &comms, VibrationAtom atom) {
    comms.send(atom.get_effect());
}

template<>
void AtomPlayer<ForceFeedbackAtom>::play(contactci::comms::Communicator &comms, ForceFeedbackAtom atom) {
    comms.send(atom.get_amplitude() * 255);
}

template<>
void AtomPlayer<PressureAtom>::play(contactci::comms::Communicator &comms, PressureAtom atom) {
    comms.send(atom.get_amplitude() * 255);
}

template<>
VibrationAtom Atom<VibrationAtom>::get_zero() {
    return VibrationAtom::ZERO;
}

template<>
ForceFeedbackAtom Atom<ForceFeedbackAtom>::get_zero() {
    return ForceFeedbackAtom::ZERO;
}

template<>
PressureAtom Atom<PressureAtom>::get_zero() {
    return PressureAtom::ZERO;
}

int main() {
    auto vs = VibrationAtom(52);
    auto fs = ForceFeedbackAtom(175);
    auto ps = PressureAtom(0);

    HapticEffect<ForceFeedbackAtom> asdf(ForceFeedbackAtom(0));

    HapticEffect<VibrationAtom, ForceFeedbackAtom, PressureAtom> temp3(vs, fs, ps);

    DebugCommunicator comms;

    // ramp
    // curves / splines
    // fade = ramp/curve toward zero
    // windowed diffuse
//
//    auto temp4 = temp3.sleep(5);
//
//    EffectPlayer::play(comms, temp4);
//    auto temp = ForceFeedbackAtom(200);
//
//    if (temp != ForceFeedbackAtom::get_zero()) {
//        std::cout << "wtf" << std::endl;
//    }

//TODO add repeat to Atom? Makes Atom aware of HapticEffect<Atom>; not sure if good

    auto asdf3 =
        HapticEffect<ForceFeedbackAtom>(ForceFeedbackAtom(0))
            .repeat(5)
            .then(ForceFeedbackAtom(1.0f))
            .map([](ForceFeedbackAtom atom) {
                if (atom != ForceFeedbackAtom::get_zero()) {
                    return atom;
                }

                return ForceFeedbackAtom(100);
            })
            .sleep(3)
            .then(ForceFeedbackAtom(0.5f))
        .join(
            HapticEffect<VibrationAtom>(VibrationAtom(52))
            .delay(5)
        );


    auto fdaf = HapticEffect<ForceFeedbackAtom>(ForceFeedbackAtom(0)).interpolate(ForceFeedbackAtom(1.0), 10, interpolation::ramp);

    EffectPlayer::play(comms, fdaf);

    return 0;
}