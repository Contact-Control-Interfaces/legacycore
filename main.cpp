//
// Created by john_contactci on 2/3/2022.
//

#include <core/haptics/effects/effect.h>
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

    bool equals(VibrationAtom other) const override {
        return effect == other.effect;
    }

    // TODO implmenetation doesn't make sense with pre-defined effects
    VibrationAtom lerp(VibrationAtom to, double t) const override {
        const uint8_t diff = to.effect - this->effect;
        return VibrationAtom(this->effect + (diff * t));
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

    ForceFeedbackAtom lerp(ForceFeedbackAtom to, double t) const override {
        const float diff = to.amplitude - this->amplitude;
        return ForceFeedbackAtom(this->amplitude + (diff * t));
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

    PressureAtom lerp(PressureAtom to, double t) const override {
        const float diff = to.amplitude - this->amplitude;
        return PressureAtom(this->amplitude + (diff * t));
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

    Effect<ForceFeedbackAtom> asdf(ForceFeedbackAtom(0));

    Effect<VibrationAtom, ForceFeedbackAtom, PressureAtom> temp3(vs, fs, ps);

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

//TODO add repeat to Atom? Makes Atom aware of Effect<Atom>; not sure if good

    auto asdf3 =
        Effect<ForceFeedbackAtom>(ForceFeedbackAtom(0))
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
                Effect<VibrationAtom>(VibrationAtom(52))
            .delay(5)
        );


    auto fdaf = Effect<ForceFeedbackAtom>(ForceFeedbackAtom(1.0)).dampen(10, interpolation::ease_in_out);

    EffectPlayer::play(comms, fdaf);

    return 0;
}