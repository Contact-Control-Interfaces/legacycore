//
// Created by john_contactci on 2/8/2022.
//

#include "core/haptics/effects/player.h"

using namespace contactci::core::haptics;

template<typename T>
struct assert_false : std::false_type
{ };

template<typename S>
void DimensionedSlicePlayer<S>::play(S _) {
    static_assert(assert_false<S>::value, "No specialization available to play haptic effect of dimension");
}