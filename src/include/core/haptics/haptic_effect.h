//
// Created by john_contactci on 2/1/2022.
//

#pragma once

#include <cstdint>
#include <vector>

namespace contactci::core::haptics {

    class DimensionSlice {

    };

    template<typename T>
    class TypedDimensionSlice : DimensionSlice  {
        // TODO
    };

    class Frame {
    private:
        std::vector<std::reference_wrapper<DimensionSlice>> slices;
    };

    class HapticEffectSequence;

    class HapticEffect {
    public:
        virtual Frame get_current_frame() = 0;
        virtual void move_next_frame() = 0;
        virtual uint32_t get_duration() = 0;

        virtual HapticEffectSequence chain(HapticEffect &other) = 0;

        ~HapticEffect() = default;

    protected:
        HapticEffect() = default;
        double scalar;
    };

    template <typename T>
    class TypedHapticEffect : public HapticEffect {
    public:
        virtual T scale(double scalar) = 0;
        // TODO chain
    protected:
        TypedHapticEffect();
        // TODO
    };

    class HapticEffectSequence : public TypedHapticEffect<HapticEffectSequence> {
    public:
        HapticEffectSequence() = default;
        HapticEffectSequence(const HapticEffectSequence &other);
        HapticEffectSequence scale(double scalar) override;
        HapticEffectSequence chain(HapticEffect &other) override;

        Frame get_current_frame() override;
        void move_next_frame() override;
        uint32_t get_duration() override;
    protected:
        std::vector<std::reference_wrapper<HapticEffect>> sequence;
    };

    /// Below pattern for DimensionedHapticEffect adapted from https://stackoverflow.com/a/53112843/792779
    /// TODO: Add type constraints for subtype of Dimension

    template<class T, class... Types> class DimensionedHapticEffect;

    template <typename T>
    class DimensionedHapticEffect<T> : public TypedHapticEffect<DimensionedHapticEffect<T>> {
    public:
        DimensionedHapticEffect<T> scale(double scalar) override {
            // TODO
            return *this;
        }

        HapticEffectSequence chain(HapticEffect &other) override {
            auto sequence = HapticEffectSequence();

            return sequence.chain(*this).chain(other);
        }

        Frame get_current_frame() override {
            // TODO
            return {};
        }

        void move_next_frame() override {
            // TODO
        }

        uint32_t get_duration() override {
            // TODO
            return 0;
        }

    protected:
        virtual std::vector<T> get_dimension() {
            return dimension;
        }

        virtual void set_dimension(const std::vector<T> dimension) {
            this->dimension = dimension;
        }
    private:
        std::vector<T> dimension;
    };

    template <typename T, typename... Types>
    class DimensionedHapticEffect : public DimensionedHapticEffect<T>, public DimensionedHapticEffect<Types...> {
    public:
        template<class V>
        std::vector<V> get_dimension() {
            return this->DimensionedHapticEffect<V>::get_dimension();
        }

        template<class V>
        void set_dimension(const std::vector<V> dimension) {
            this->DimensionedHapticEffect<V>::set_dimension(dimension);
        }
    };

    class VibrationDimension {};
    class ForceFeedbackDimension {};

    void usage_example() {
        DimensionedHapticEffect<VibrationDimension, ForceFeedbackDimension> effect;

        effect.set_dimension(std::vector<VibrationDimension> {VibrationDimension(), VibrationDimension()});
        effect.set_dimension(std::vector<ForceFeedbackDimension> {ForceFeedbackDimension()});

        std::vector<ForceFeedbackDimension> works_fine = effect.get_dimension<ForceFeedbackDimension>();
        //std::vector<int> blows_up = effect.get_dimension<int>();
    }
}

template <typename T>
contactci::core::haptics::TypedHapticEffect<T>::TypedHapticEffect() {
    static_assert(std::is_base_of<HapticEffect, T>::value);
}
