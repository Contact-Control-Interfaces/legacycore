//
// Created by john_contactci on 2/3/2022.
//

#pragma once

#include "core/haptics/effects/haptic_effect.h"
#include "core/haptics/dimension.h"

namespace contactci::core::haptics::effects {

    // Below pattern for DimensionedHapticEffect adapted from https://stackoverflow.com/a/53112843/792779

    template<DimensionDerived D, DimensionDerived... Ds>
    class DimensionedHapticEffect;

    template<DimensionDerived D>
    class DimensionedHapticEffect<D> : public TypedHapticEffect<DimensionedHapticEffect<D>> {
    public:
        DimensionedHapticEffect<D> scale(double scalar) override {
            this->scalar = scalar;

            return *this;
        }

        HapticEffectSequence chain(HapticEffect &other) override {
            return HapticEffectSequence(*this).chain(other);
        }

        DimensionedFrame <D> &get_current_frame() override {
            return current_frame;
        }

        void move_next_frame() override {
            // TODO
        }

        uint32_t get_duration() override {
            // TODO
            return 0;
        }

    protected:
        virtual std::vector <D> get_dimension() {
            return dimension;
        }

        virtual void set_dimension(const std::vector <D> dimension) {
            this->dimension = dimension;
        }

    private:
        std::vector <D> dimension;
        DimensionedFrame <D> current_frame;
    };

    template<DimensionDerived D, DimensionDerived... Ds>
    class DimensionedHapticEffect : public DimensionedHapticEffect<D>, public DimensionedHapticEffect<Ds...> {
    public:
        template<class V>
        std::vector <V> get_dimension() {
            return this->DimensionedHapticEffect<V>::get_dimension();
        }

        template<class V>
        void set_dimension(const std::vector <V> dimension) {
            this->DimensionedHapticEffect<V>::set_dimension(dimension);
        }

        DimensionedFrame<D, Ds...> &get_current_frame() override {
            return current_frame;
        }

    private:
        DimensionedFrame<D, Ds...> current_frame;
    };
}