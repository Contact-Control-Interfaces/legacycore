//
// Created by john_contactci on 2/1/2022.
//

#include "core/haptics/dimension.h"

using namespace contactci::core::haptics;

Dimension::Dimension() : Dimension(0, UINT32_MAX) { }
Dimension::Dimension(uint32_t delay, uint32_t duration) : delay(delay), duration(duration) { }

uint32_t Dimension::get_length() const {
    return delay + duration;
}