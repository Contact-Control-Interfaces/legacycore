//
// Created by john_contactci on 2/7/2022.
//

#pragma once

#include "core/haptics/dimension.h"

namespace contactci::core::haptics {

    // TODO enforce that S is a subclass of TypedDimensionSlice
    template<typename D>
    class DimensionedSlicePlayer {
    public:
        static void play(TypedDimensionSlice<D> &slice);
    };
}