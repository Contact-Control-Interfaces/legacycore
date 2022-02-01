//
// Created by john_contactci on 2/1/2022.
//

#pragma once

#include <cstdint>
#include <type_traits>

namespace contactci::core::haptics {

    class Dimension {
    public:
        uint32_t get_length() const;
        Dimension();

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