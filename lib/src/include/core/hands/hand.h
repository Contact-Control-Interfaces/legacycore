//
// Created by john_contactci on 1/31/2022.
//

#pragma once

#include "hand_tree.h"

#include <tuple>

namespace contactci::core::hands {

    enum class WhichHand {
        Left,
        Right
    };

    template <typename EffectType, typename... OtherDataTypes>
    class Hand {
    public:
        ~Hand() = default;

        template <typename T>
        T get_value_at(HandTreeIndex index) const;

        template <typename T>
        void set_value_at(HandTreeIndex index, T value);

    protected:
        Hand(WhichHand which_hand);

    private:
        HandTree<std::tuple<EffectType, OtherDataTypes...>> hand_tree;
        WhichHand which_hand;
    };

    template <typename EffectType, typename... OtherDataTypes>
    class RightHand : public Hand<EffectType, OtherDataTypes...> {
    public:
        RightHand();
        ~RightHand() = default;
    };

    template <typename EffectType, typename... OtherDataTypes>
    class LeftHand : public Hand<EffectType, OtherDataTypes...> {
    public:
        LeftHand();
        ~LeftHand() = default;
    };

    // TODO Build tree? Allow empty nodes and/or no root?
    template <typename EffectType, typename... OtherDataTypes>
    Hand<EffectType, OtherDataTypes...>::Hand(WhichHand which_hand)
            : which_hand(which_hand), hand_tree(HandTree<std::tuple<EffectType, OtherDataTypes...>>()) { }

    template <typename EffectType, typename... OtherDataTypes>
    template <typename T>
    T Hand<EffectType, OtherDataTypes...>::get_value_at(HandTreeIndex index) const {
        return std::get<T>(hand_tree.get_node(index).get_value());
    }

    template <typename EffectType, typename... OtherDataTypes>
    template <typename T>
    void Hand<EffectType, OtherDataTypes...>::set_value_at(HandTreeIndex index, T value) {
        std::get<T>(hand_tree.get_node(index).get_value()) = value;
    }

    template <typename EffectType, typename... OtherDataTypes>
    RightHand<EffectType, OtherDataTypes...>::RightHand() : Hand<EffectType, OtherDataTypes...>(WhichHand::Right) { }

    template <typename EffectType, typename... OtherDataTypes>
    LeftHand<EffectType, OtherDataTypes...>::LeftHand() : Hand<EffectType, OtherDataTypes...>(WhichHand::Left) { }
}
