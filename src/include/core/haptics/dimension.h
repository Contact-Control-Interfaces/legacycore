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

    template<typename T>
    class TypedDimensionSlice : public DimensionSlice  {
        // TODO
    };

    class Dimension {
    public:
        Dimension();

        uint32_t get_delay() const;
        uint32_t get_duration() const;
        uint32_t get_length() const;

    protected:
        Dimension(uint32_t delay, uint32_t duration);

        uint32_t delay;
        uint32_t duration;
    };

    //TODO introduce type between Dimension and TemporalDimension named TypedDimension?
    // Dimension -> TypedDimension -> TemporalDimension

    // Using DimensionDerived here causes incomplete type issues when trying to extend TemporalDimension
    template <typename T>
    class TemporalDimension : public Dimension {
    public:
        TemporalDimension(uint32_t delay, uint32_t duration);

        virtual TypedDimensionSlice<T> get_slice(uint32_t offset) = 0;
    };

    template <typename T>
    TemporalDimension<T>::TemporalDimension(uint32_t delay, uint32_t duration)
            : Dimension(delay, duration) {
        static_assert(std::is_base_of<Dimension, T>::value);
    }
}

