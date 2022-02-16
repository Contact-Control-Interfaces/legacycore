//
// Created by john_contactci on 2/8/2022.
//

#include "core/haptics/effects/player.h"

using namespace contactci::core::haptics;

template<typename T>
struct assert_false : std::false_type
{ };

template<typename D>
void DimensionedSlicePlayer<D>::play(TypedDimensionSlice<D> &_) {
    static_assert(assert_false<D>::value, "No specialization available to play haptic effect of dimension");
}
