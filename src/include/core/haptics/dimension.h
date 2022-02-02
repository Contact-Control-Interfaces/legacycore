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

    template<DimensionDerived T>
    class TypedDimensionSlice : DimensionSlice  {
        // TODO
    };

    class Dimension {
    public:
        Dimension();

        virtual DimensionSlice get_slice(uint32_t offset) = 0;
        uint32_t get_length() const;

    protected:
        Dimension(uint32_t delay, uint32_t duration);

        uint32_t delay;
        uint32_t duration;
    };

    template <DimensionDerived T>
    class TemporalDimension : Dimension {
    public:
        TemporalDimension(T &dimension, uint32_t delay, uint32_t duration);

    private:
        T &dimension;
    };

    template <DimensionDerived T>
    TemporalDimension<T>::TemporalDimension(T &dimension, uint32_t delay, uint32_t duration)
            : Dimension(delay, duration), dimension(dimension)
    { }
}

