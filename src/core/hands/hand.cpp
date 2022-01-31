//
// Created by john_contactci on 1/31/2022.
//

#include "../../include/core/hands/hand.h"

contactci::core::hands::Hand::Hand(WhichHand which_hand)
    : which_hand(which_hand), hand_tree(HandTree()) { }

contactci::core::hands::Hand::~Hand() = default;

contactci::core::hands::RightHand::RightHand() : Hand(WhichHand::Right) { }

contactci::core::hands::RightHand::~RightHand() = default;

contactci::core::hands::LeftHand::LeftHand() : Hand(WhichHand::Left) { }

contactci::core::hands::LeftHand::~LeftHand() = default;