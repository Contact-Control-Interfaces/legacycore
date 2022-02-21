//
// Created by john_contactci on 2/7/2022.
//

#pragma once

#include "core/haptics/dimension.h"
#include "core/haptics/frame.h"
#include "core/haptics/effects/haptic_effect.h"

#include <iostream>
#include <comms/communicator.h>

namespace contactci::core::haptics {

    // TODO enforce that S is a subclass of TypedDimensionSlice
    template<typename D>
    class DimensionedSlicePlayer {
    public:
        static void play(contactci::comms::Communicator &comms, TypedDimensionSlice<D> &slice);
    };

    template <typename D, typename ...Ds>
    class FramePlayer {
    public:
        static void play(contactci::comms::Communicator &comms, DimensionedFrame<D, Ds...> &frame);
    };

    class EffectPlayer {
    public:
        template <typename D, typename ...Ds>
        static void play(contactci::comms::Communicator &comms, effects::HapticEffect<D, Ds...> effect);
    };

    template <typename D, typename ...Ds>
    void contactci::core::haptics::EffectPlayer::play(contactci::comms::Communicator &comms, effects::HapticEffect<D, Ds...> effect) {
        for (auto frame : effect.get_frames()) {
            FramePlayer<D, Ds...>::play(comms, frame);
        }
    }

    template <typename D, typename ...Ds>
    void contactci::core::haptics::FramePlayer<D, Ds...>::play(contactci::comms::Communicator &comms, DimensionedFrame<D, Ds...> &frame) {
        comms.start_frame();

        play_frame<D, Ds...>(comms, frame);

        comms.end_frame();
    }

    template <typename D, typename ...Ds>
    static void play_frame(contactci::comms::Communicator &comms, DimensionedFrame<D, Ds...> &frame) {
        DimensionedSlicePlayer<D>::play(comms, frame.DimensionedFrame<D>::get_dimension_slice());
        (DimensionedSlicePlayer<Ds>::play(comms, frame.DimensionedFrame<Ds>::get_dimension_slice()), ...);
    }
}