//
// Created by john_contactci on 1/31/2022.
//

#pragma once

#include "hands/hand.h"
#include "haptics/effects/effect.h"
#include "haptics/effects/player.h"

#include "io/channel.h"

#include <optional>

namespace contactci::core {

    using namespace contactci::core::hands;
    using namespace contactci::core::haptics;
    using namespace contactci::core::haptics::effects;
    using namespace contactci::io;

    template <typename A, typename... As>
    class UserHandState;

    // TODO probably move this logic to player.h somehwere
    template <typename A, typename... As>
    static inline void update_hand(Hand<UserHandState<A, As...>> &hand, Channel &channel) {
        hand.template for_each_value([&channel](UserHandState<A, As...> &hand_state) {
            if (hand_state.playing_effect.has_value()) {
                PlayingEffect<A, As...> &playing_effect = *hand_state.playing_effect;

                if (playing_effect.is_at_end()) {
                    hand_state.clear_effect();
                } else {
                    Frame<A, As...> current_frame = playing_effect.get_current_and_increment();

                    FramePlayer<A, As...>::play(channel, current_frame);
                }
            }
        });
    }

    template <typename A, typename... As>
    class UserHandState {
    public:
        UserHandState() = default;
        ~UserHandState() = default;

        UserHandState(const UserHandState<A, As...> &other) : playing_effect(other.playing_effect) { }

        UserHandState(effects::Effect<A, As...> &effect)
                : playing_effect(std::make_optional<PlayingEffect<A, As...>>(effect)) { }

        void set_effect(
            effects::Effect<A, As...> &effect,
            void (*on_effect_completed)(effects::Effect<A, As...>*) = nullptr
        ) {
            playing_effect = std::make_optional<PlayingEffect<A, As...>>(effect, on_effect_completed);
        }

        void clear_effect() {
            playing_effect->finish();
            playing_effect.reset();
        }

        friend void update_hand<A, As...>(Hand<UserHandState<A, As...>> &hand, Channel &channel);

    private:
        std::optional<PlayingEffect<A, As...>> playing_effect;
    };

    template <typename A, typename... As>
    class User {
    public:
        ~User() = default;

        // Singleton for now; maybe eventually support multiplayer
        static User<A, As...> current_user;

        LeftHand<UserHandState<A, As...>> leftHand;
        RightHand<UserHandState<A, As...>> rightHand;

        void apply_effect(
            WhichHand handedness,
            const HandTreeIndex &index,
            effects::Effect<A, As...> &effect,
            void (*on_effect_completed)(effects::Effect<A, As...>*) = nullptr
        );

        void update(Channel &channel);
    private:
        User();
    };

    template <typename A, typename... As>
    User<A, As...> User<A, As...>::current_user;

    template <typename A, typename... As>
    User<A, As...>::User() : leftHand(LeftHand<UserHandState<A, As...>>()), rightHand(RightHand<UserHandState<A, As...>>()) { }

    template <typename A, typename... As>
    void User<A, As...>::apply_effect(
            WhichHand handedness,
            const HandTreeIndex &index,
            effects::Effect<A, As...> &effect,
            void (*on_effect_completed)(effects::Effect<A, As...>*)
    ) {
        auto &hand = handedness == WhichHand::Left
                ? static_cast<Hand<UserHandState<A, As...>>&>(leftHand)
                : static_cast<Hand<UserHandState<A, As...>>&>(rightHand);

        UserHandState<A, As...> &state = hand.get_value_at(index);

        state.set_effect(effect, on_effect_completed);
    }

    template <typename A, typename... As>
    void User<A, As...>::update(Channel &channel) {
        update_hand(leftHand, channel);
        update_hand(rightHand, channel);
    }
}
