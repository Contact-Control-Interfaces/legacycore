//
// Created by john_contactci on 2/1/2022.
//

#include "core/haptics/dimension.h"

using namespace contactci::core::haptics;

Dimension::Dimension() : Dimension(UINT32_MAX) { }
Dimension::Dimension(uint32_t duration) : duration(duration) { }

uint32_t Dimension::get_duration() const {
    return duration;
}