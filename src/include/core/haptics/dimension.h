//
// Created by john_contactci on 2/1/2022.
//

#pragma once

#include <cstdint>
#include <type_traits>

namespace contactci::core::haptics {

    class Dimension;

    template<class T>
    concept DimensionDerived = std::is_base_of<Dimension, T>::value;

    class DimensionSlice {

    };

    template<class T>
    concept DimensionSliceDerived = std::is_base_of<DimensionSlice, T>::value;

    template<typename T>
    class TypedDimensionSlice : public DimensionSlice  {
        virtual TypedDimensionSlice<T> &get_zero() const = 0;
    };

    class Dimension {
    public:
        Dimension();

        uint32_t get_duration() const;

    protected:
        Dimension(uint32_t duration);

        uint32_t duration;
    };

    //TODO introduce type between Dimension and TemporalDimension named TypedDimension?
    // Dimension -> TypedDimension -> TemporalDimension

    // Using DimensionDerived here causes incomplete type issues when trying to extend TemporalDimension
    template <typename T>
    class TemporalDimension : public Dimension {
    public:
        TemporalDimension(uint32_t duration);

        virtual TypedDimensionSlice<T> get_slice(uint32_t offset) = 0;

        // { amplitude: 100 }.wrap.scale(duration)
        // wrap (Slice) -> HapticEffect
    };

    template <typename T>
    TemporalDimension<T>::TemporalDimension(uint32_t duration)
            : Dimension(duration) {
        static_assert(std::is_base_of<Dimension, T>::value);
    }
}

