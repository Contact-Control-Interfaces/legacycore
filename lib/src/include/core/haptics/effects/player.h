//
// Created by john_contactci on 2/7/2022.
//

#pragma once

#include "core/haptics/dimension.h"
#include "core/haptics/frame.h"

namespace contactci::core::haptics {

    // TODO enforce that S is a subclass of TypedDimensionSlice
    template<typename D>
    class DimensionedSlicePlayer {
    public:
        static void play(TypedDimensionSlice<D> &slice);
    };

    template <typename D, typename ...Ds>
    class FramePlayer {
    public:
        static void play(DimensionedFrame<D, Ds...> &frame);
    };

    template <typename D, typename ...Ds>
    class EffectPlayer {
    public:
        //static void play(HapticEffect hapticEffect) {
        //    // TODO hapticEffect.get_frames
        //}
    };

    template <typename D, typename ...Ds>
    void contactci::core::haptics::FramePlayer<D, Ds...>::play(DimensionedFrame<D, Ds...> &frame) {
        if constexpr (sizeof...(Ds) == 0) {
            // This is the base case where Ts is empty and we just have template arg T
            // Since the condition is a constexpr, the else block is completely discarded at compile-time when it evaluates to true
            // avoiding the base case issue of unpacking empty Ts
            DimensionedSlicePlayer<D>::play(frame.DimensionedFrame<D>::get_dimension_slice());
        } else { // This else needs to be here explicitly to avoid issues deducing template argument T for base case
            // Recurse on remaining dimension types Ts
            play<Ds...>(frame);
        }
    }


}