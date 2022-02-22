//
// Created by john_contactci on 2/8/2022.
//

#include "core/haptics/effects/player.h"

#include <iostream>

using namespace contactci::core::haptics;

template<typename T>
struct assert_false : std::false_type
{ };

template<typename A>
void AtomPlayer<A>::play(contactci::comms::Communicator &comms, A &_) {
    static_assert(assert_false<A>::value, "No specialization available to play haptic effect of atom type");
}
