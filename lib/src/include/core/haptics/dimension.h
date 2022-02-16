//
// Created by john_contactci on 2/1/2022.
//

#pragma once

#include <cstdint>
#include <type_traits>

#define DimensionDerived typename

namespace contactci::core::haptics {

    class DimensionSlice {

    };

    template<class T>
    concept DimensionSliceDerived = std::is_base_of<DimensionSlice, T>::value;

    template<typename D>
    class TypedDimensionSlice;

    // TODO Look into enforcing that S is S<D>

    template<typename D>
    class TypedDimensionSlice : public DimensionSlice  {
    public:
        static TypedDimensionSlice<D> &get_zero();
    };

    template <typename D>
    class Dimension {
    public:
        Dimension();

        uint32_t get_duration() const;
        virtual TypedDimensionSlice<D> &get_slice(uint32_t offset) = 0;

    protected:
        explicit Dimension(uint32_t duration);

        uint32_t duration;
    };

    template <typename D>
    Dimension<D>::Dimension() : Dimension(UINT32_MAX) { }

    template <typename D>
    Dimension<D>::Dimension(uint32_t duration) : duration(duration) {
        static_assert(std::is_base_of<Dimension, D>::value);
    }

    template <typename D>
    uint32_t Dimension<D>::get_duration() const {
        return duration;
    }
}
