//
// Created by john_contactci on 1/31/2022.
//

#pragma once

#include "core/hands/hand.h"

namespace contactci::core {

    using namespace contactci::core::hands;

    class User {
    public:
        ~User();

        // Singleton for now; maybe eventually support multiplayer
        static const User current_user;

    private:
        const LeftHand leftHand;
        const RightHand rightHand;
        User();
    };
}
