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

    template <typename HandState>
    class Hand {
    public:
        ~Hand() = default;

        HandState &get_value_at(HandTreeIndex index);

        void set_value_at(HandTreeIndex index, HandState &value);

        template <typename Func>
        void for_each_value(const Func &&func) {
            hand_tree.for_each_node(func);
        }

    protected:
        Hand(WhichHand which_hand);

    private:
        HandTree<HandState> hand_tree;
        WhichHand which_hand;
    };

    template <typename HandState>
    class RightHand : public Hand<HandState> {
    public:
        RightHand();
        ~RightHand() = default;
    };

    template <typename HandState>
    class LeftHand : public Hand<HandState> {
    public:
        LeftHand();
        ~LeftHand() = default;
    };

    template <typename HandState>
    Hand<HandState>::Hand(WhichHand which_hand) : which_hand(which_hand), hand_tree(HandTree<HandState>()) { }

    template <typename HandState>
    HandState &Hand<HandState>::get_value_at(HandTreeIndex index) {
        return hand_tree.get_node(index).get_value();
    }

    template <typename HandState>
    void Hand<HandState>::set_value_at(HandTreeIndex index, HandState &value) {
        hand_tree.get_node(index).set_value(value);
    }

    template <typename HandState>
    RightHand<HandState>::RightHand() : Hand<HandState>(WhichHand::Right) { }

    template <typename HandState>
    LeftHand<HandState>::LeftHand() : Hand<HandState>(WhichHand::Left) { }
}
