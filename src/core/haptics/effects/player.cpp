//
// Created by john_contactci on 2/8/2022.
//

#include "core/haptics/effects/player.h"

using namespace contactci::core::haptics;

template<DimensionDerived T>
struct assert_false : std::false_type
{ };

template<DimensionDerived T>
void DimensionedSlicePlayer<T>::play(TypedDimensionSlice<T> _) {
    static_assert(assert_false<T>::value, "No specialization available to play haptic effect of dimension");
}