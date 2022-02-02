//
// Created by john_contactci on 2/1/2022.
//

#pragma once

#include <cstdint>
#include <vector>

#include "dimension.h"
#include "frame.h"

namespace contactci::core::haptics {
    class HapticEffectSequence;

    class HapticEffect {
    public:
        virtual Frame &get_current_frame() = 0;
        virtual void move_next_frame() = 0;
        virtual uint32_t get_duration() = 0;

        virtual HapticEffectSequence chain(HapticEffect &other) = 0;

        ~HapticEffect() = default;

    protected:
        explicit HapticEffect(double scalar = 1.0);
        double scalar;
    };

    template<class T>
    concept HapticEffectDerived = std::is_base_of<HapticEffect, T>::value;

    template <typename T>
    class TypedHapticEffect : public HapticEffect {
    public:
        virtual T scale(double scalar) = 0;
    protected:
        explicit TypedHapticEffect(double scalar = 1.0);
    };

    class HapticEffectSequence : public TypedHapticEffect<HapticEffectSequence> {
    public:
        explicit HapticEffectSequence(HapticEffect &initial);
        HapticEffectSequence(const HapticEffectSequence &other);
        HapticEffectSequence scale(double scalar) override;
        HapticEffectSequence chain(HapticEffect &other) override;

        Frame &get_current_frame() override;
        void move_next_frame() override;
        uint32_t get_duration() override;
    protected:
        std::vector<std::reference_wrapper<HapticEffect>> sequence;
        HapticEffect &current_effect;
    };

    /// Below pattern for DimensionedHapticEffect adapted from https://stackoverflow.com/a/53112843/792779
    /// TODO: Add type constraints for subtype of Dimension

    template<DimensionDerived D, DimensionDerived... Ds> class DimensionedHapticEffect;

    template <DimensionDerived D >
    class DimensionedHapticEffect<D> : public TypedHapticEffect<DimensionedHapticEffect<D>> {
    public:
        DimensionedHapticEffect<D> scale(double scalar) override {
            this->scalar = scalar;

            return *this;
        }

        HapticEffectSequence chain(HapticEffect &other) override {
            return HapticEffectSequence(*this).chain(other);
        }

        DimensionedFrame<D> &get_current_frame() override {
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
        virtual std::vector<D> get_dimension() {
            return dimension;
        }

        virtual void set_dimension(const std::vector<D> dimension) {
            this->dimension = dimension;
        }
    private:
        std::vector<D> dimension;
        DimensionedFrame<D> current_frame;
    };

    template <DimensionDerived D, DimensionDerived... Ds>
    class DimensionedHapticEffect : public DimensionedHapticEffect<D>, public DimensionedHapticEffect<Ds...> {
    public:
        template<class V>
        std::vector<V> get_dimension() {
            return this->DimensionedHapticEffect<V>::get_dimension();
        }

        template<class V>
        void set_dimension(const std::vector<V> dimension) {
            this->DimensionedHapticEffect<V>::set_dimension(dimension);
        }

        DimensionedFrame<D, Ds...> &get_current_frame() override {
            return current_frame;
        }

    private:
        DimensionedFrame<D, Ds...> current_frame;
    };

    class VibrationDimension : Dimension {
    public:
        DimensionSlice get_slice(uint32_t offset) override {
            return {};
        }
    };

    class ForceFeedbackDimension : Dimension {
    public:
        DimensionSlice get_slice(uint32_t offset) override {
            return {};
        }
    };

    void usage_example() {
        DimensionedHapticEffect<VibrationDimension, ForceFeedbackDimension> effect;

        effect.set_dimension(std::vector<VibrationDimension> {VibrationDimension(), VibrationDimension()});
        effect.set_dimension(std::vector<ForceFeedbackDimension> {ForceFeedbackDimension()});

        std::vector<ForceFeedbackDimension> works_fine = effect.get_dimension<ForceFeedbackDimension>();
        //std::vector<int> blows_up = effect.get_dimension<int>();
    }
}

template <typename T>
contactci::core::haptics::TypedHapticEffect<T>::TypedHapticEffect(double scalar)
        : contactci::core::haptics::HapticEffect(scalar) {
    static_assert(std::is_base_of<HapticEffect, T>::value);
}
