//
// Created by john_contactci on 2/16/2022.
//

#include "core/haptics/dimension.h"

using namespace contactci::core::haptics;

template<typename T>
struct assert_false : std::false_type
{ };

template<typename D>
TypedDimensionSlice<D> &TypedDimensionSlice<D>::get_zero() {
    static_assert(assert_false<D>::value, "No specialization available to get zero slice for dimension");
}

template <typename D>
char Dimension<D>::get_representation() {
    return '_';
}