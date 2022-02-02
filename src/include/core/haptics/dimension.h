//
// Created by john_contactci on 2/1/2022.
//

#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>

namespace contactci::core::haptics {

    class DimensionSlice {

    };

    template<typename T>
    class TypedDimensionSlice : DimensionSlice  {
        // TODO
    };

    class Frame {

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

    template <typename T>
    class TemporalDimension : Dimension {
    public:
        TemporalDimension(T &dimension, uint32_t delay, uint32_t duration);

    private:
        T &dimension;
    };
}

template <typename T>
contactci::core::haptics::TemporalDimension<T>::TemporalDimension(T &dimension, uint32_t delay, uint32_t duration)
        : Dimension(delay, duration), dimension(dimension) {
    static_assert(std::is_base_of<Dimension, T>::value);
}