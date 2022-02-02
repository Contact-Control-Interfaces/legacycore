//
// Created by john_contactci on 2/1/2022.
//

#pragma once

#include <cstdint>
#include <vector>
#include <iterator>

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
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = HapticEffect;
        using pointer           = value_type*;
        using reference         = std::reference_wrapper<value_type>;
    public:
        HapticEffectSequence(const HapticEffectSequence &other);
        HapticEffectSequence scale(double scalar) override;
        HapticEffectSequence chain(HapticEffect &other) override;

        Frame get_current_frame() override;
        void move_next_frame() override;
        uint32_t get_duration() override;
    protected:
        std::vector<reference> sequence;
    };
}

template <typename T>
contactci::core::haptics::TypedHapticEffect<T>::TypedHapticEffect() {
    static_assert(std::is_base_of<HapticEffect, T>::value);
}
