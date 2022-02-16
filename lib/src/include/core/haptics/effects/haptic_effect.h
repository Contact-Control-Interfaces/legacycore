//
// Created by john_contactci on 2/1/2022.
//

#pragma once

#include <cstdint>
#include <vector>

#include "core/haptics/frame.h"

namespace contactci::core::haptics::effects {

    class ZeroHapticEffect;

    // TODO Sequence iterator of effects
    // Effect iterator of frames
    // Player gets an effect, iterates across the frames, calls the template specialized overlaod of play function
    // for the dimension slices of the frame

    // TODO HapticEffect is a container of frames
    // Needs to return a collection of Frames (iterator?)
    class HapticEffect {
    public:
        static ZeroHapticEffect &ZERO;

        virtual FrameIterator get_frames() const = 0;

        virtual uint32_t get_duration() const;
        virtual double get_scalar() const;

        virtual HapticEffectSequence chain(HapticEffect &other) = 0;

        ~HapticEffect() = default;

        friend bool operator== (const HapticEffect& a, const HapticEffect& b) {
            return a.scalar == b.scalar;
                // TODO actually compare effects
        };

        friend bool operator!= (const HapticEffect& a, const HapticEffect& b) {
            return !(a == b);
        };

    protected:
        explicit HapticEffect(uint32_t duration, double scalar = 1.0);

        uint32_t duration;
        double scalar;
    };

    class ZeroHapticEffect : public HapticEffect {
    public:
        ZeroHapticEffect();
        ZeroHapticEffect(const ZeroHapticEffect &other);
        ZeroHapticEffect(uint32_t duration);

        FrameIterator get_frames() const override;

        HapticEffectSequence chain(HapticEffect &other) override;
        ZeroHapticEffect scale(double scalar);
    private:
        NullFrame null_frame;
        uint32_t duration = 1;
    };
}
