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

    template<typename D>
    class TypedDimensionSlice;

    // TODO Look into enforcing that S is S<D>

    template<typename D>
    class TypedDimensionSlice : public DimensionSlice  {
        virtual TypedDimensionSlice<D> &get_zero() const = 0;
    };

    class Dimension {
    public:
        Dimension();

        uint32_t get_duration() const;

    protected:
        explicit Dimension(uint32_t duration);

        uint32_t duration;
    };

    //TODO introduce type between Dimension and TemporalDimension named TypedDimension?
    // Dimension -> TypedDimension -> TemporalDimension

    // Using DimensionDerived here causes incomplete type issues when trying to extend TemporalDimension
    template <typename D>
    class TemporalDimension : public Dimension {
    public:
        explicit TemporalDimension(uint32_t duration);

        virtual TypedDimensionSlice<D> &get_slice(uint32_t offset) = 0;
    };

    template <typename D>
    TemporalDimension<D>::TemporalDimension(uint32_t duration)
            : Dimension(duration) {
        static_assert(std::is_base_of<Dimension, D>::value);
    }
}

