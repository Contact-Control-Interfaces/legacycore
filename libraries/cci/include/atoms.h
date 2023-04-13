//
// Created by john_contactci on 3/9/2022.
//

#pragma once

#include <cci_lib_defs.h>

#include <core/haptics/frame.h>

#include <cstdint>

namespace contactci::core::haptics::atoms {

    using namespace contactci::core::haptics;

    enum HapticDimension {
        Invalid = 0,
        ForceFeedback = 1,
        Vibration = 2
    };

    class CCI_API_CLASS(VibrationAtom) : public Atom<VibrationAtom> {
    public:
        static VibrationAtom ZERO;

        explicit VibrationAtom(uint8_t effect);

        bool equals(VibrationAtom other) const override;

        // TODO implementation doesn't make sense with pre-defined effects
        VibrationAtom lerp(VibrationAtom to, double t) const override;

        uint8_t get_effect();

    private:
        uint8_t effect;
    };

    class CCI_API_CLASS(ForceFeedbackAtom) : public Atom<ForceFeedbackAtom> {
    public:
        static ForceFeedbackAtom ZERO;

        explicit ForceFeedbackAtom(float amplitude);

        ForceFeedbackAtom lerp(ForceFeedbackAtom to, double t) const override;

        bool equals(ForceFeedbackAtom other) const override;

        float get_amplitude();

    private:
        float amplitude; // 0 - 1
    };
}
