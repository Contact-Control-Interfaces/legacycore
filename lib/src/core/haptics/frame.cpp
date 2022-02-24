//
// Created by john_contactci on 2/2/2022.
//

#include "core/haptics/frame.h"

#include <type_traits>

using namespace contactci::core::haptics;

template<typename T>
struct assert_false : std::false_type
{ };

template<typename A>
A Atom<A>::get_zero() {
    static_assert(assert_false<A>::value, "No specialization available to get zero atom for atom type");
}