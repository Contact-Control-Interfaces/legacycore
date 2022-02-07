//
// Created by john_contactci on 2/7/2022.
//

#pragma once

#include "core/haptics/dimension.h"

namespace contactci::core::haptics {

    template<DimensionDerived S>
    class DimensionedSlicePlayer {
    public:
        static void play(TypedDimensionSlice<S> slice);
    };
}