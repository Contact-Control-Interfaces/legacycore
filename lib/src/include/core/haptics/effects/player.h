//
// Created by john_contactci on 2/7/2022.
//

#pragma once

#include "core/user.h"
#include "core/haptics/frame.h"
#include "core/haptics/effects/effect.h"

#include <iostream>
#include <comms/communicator.h>

namespace contactci::core::haptics {

    // TODO enforce that S is a subclass of Atom
    template<typename A>
    class AtomPlayer {
    public:
        static void play(contactci::comms::Communicator &comms, A atom);
    };

    template <typename A, typename ...As>
    class FramePlayer {
    public:
        static void play(contactci::comms::Communicator &comms, const Frame<A, As...> &frame);
    };

    class EffectPlayer {
    public:
        template <typename A, typename ...As>
        static void play(contactci::comms::Communicator &comms, const effects::Effect<A, As...> &effect);
    };

    class UserEffectPlayer {
    public:
        template <typename A, typename... As, typename... OtherData>
        static void play(contactci::comms::Communicator &comms, const contactci::core::User<effects::Effect<A, As...>, OtherData...> &user);
    };

    template <typename A, typename... As, typename... OtherData>
    void UserEffectPlayer::play(contactci::comms::Communicator &comms, const contactci::core::User<effects::Effect<A, As...>, OtherData...> &user) {
        user.leftHand.template for_each_value<effects::Effect<A, As...>>([&comms](auto &effect) {
            EffectPlayer::play(comms, effect);
        });

        user.rightHand.template for_each_value<effects::Effect<A, As...>>([&comms](auto &effect) {
            EffectPlayer::play(comms, effect);
        });
    }

    template <typename A, typename ...As>
    void EffectPlayer::play(contactci::comms::Communicator &comms, const effects::Effect<A, As...> &effect) {
        const auto frames = effect.get_frames();

        for (auto frame : frames) {
            FramePlayer<A, As...>::play(comms, frame);
        }
    }

    template <typename A, typename ...As>
    void FramePlayer<A, As...>::play(contactci::comms::Communicator &comms, const Frame<A, As...> &frame) {
        comms.start_frame();

        play_frame<A, As...>(comms, frame);

        comms.end_frame();
    }

    template <typename A, typename ...As>
    static void play_frame(contactci::comms::Communicator &comms, const Frame<A, As...> &frame) {
        AtomPlayer<A>::play(comms, frame.Frame<A>::get_atom());
        (AtomPlayer<As>::play(comms, frame.Frame<As>::get_atom()), ...);
    }
}