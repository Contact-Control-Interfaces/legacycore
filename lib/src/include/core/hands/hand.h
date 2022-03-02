//
// Created by john_contactci on 1/31/2022.
//

#pragma once

#include "hand_tree.h"

namespace contactci::core::hands {

    enum class WhichHand {
        Left,
        Right
    };

    template <typename EffectType, typename... OtherDataTypes>
    class Hand {
    public:
        ~Hand() = default;

    protected:
        Hand(WhichHand which_hand);

    private:
        HandTree<std::tuple<EffectType, OtherDataTypes...>> hand_tree;
        WhichHand which_hand;
    };

    template <typename EffectType, typename... OtherDataTypes>
    class RightHand : Hand<EffectType, OtherDataTypes...> {
    public:
        RightHand();
        ~RightHand() = default;
    };

    template <typename EffectType, typename... OtherDataTypes>
    class LeftHand : Hand<EffectType, OtherDataTypes...> {
    public:
        LeftHand();
        ~LeftHand() = default;
    };

    // TODO Build tree? Allow empty nodes and/or no root?
    template <typename EffectType, typename... OtherDataTypes>
    Hand<EffectType, OtherDataTypes...>::Hand(WhichHand which_hand)
            : which_hand(which_hand), hand_tree(HandTree<std::tuple<EffectType, OtherDataTypes...>>()) { }

    template <typename EffectType, typename... OtherDataTypes>
    RightHand<EffectType, OtherDataTypes...>::RightHand() : Hand<EffectType, OtherDataTypes...>(WhichHand::Right) { }

    template <typename EffectType, typename... OtherDataTypes>
    LeftHand<EffectType, OtherDataTypes...>::LeftHand() : Hand<EffectType, OtherDataTypes...>(WhichHand::Left) { }
}
