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

    class Hand {
    public:
        ~Hand();

    protected:
        Hand(WhichHand which_hand);

    private:
        HandTree hand_tree;
        WhichHand which_hand;
    };

    class RightHand : Hand {
    public:
        RightHand();
        ~RightHand();
    };

    class LeftHand : Hand {
    public:
        LeftHand();
        ~LeftHand();
    };
}
