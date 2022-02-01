//
// Created by john_contactci on 1/31/2022.
//

#include "core/hands/hand.h"

using namespace contactci::core::hands;

Hand::Hand(WhichHand which_hand)
    : which_hand(which_hand), hand_tree(HandTree()) { }

Hand::~Hand() = default;

RightHand::RightHand() : Hand(WhichHand::Right) { }

RightHand::~RightHand() = default;

LeftHand::LeftHand() : Hand(WhichHand::Left) { }

LeftHand::~LeftHand() = default;