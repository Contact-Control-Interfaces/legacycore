//
// Created by john_contactci on 1/31/2022.
//

#pragma once

#include "core/hands/hand.h"

namespace contactci::core {

    using namespace contactci::core::hands;

    template <typename EffectType, typename... OtherDataTypes>
    class User {
    public:
        ~User() = default;

        // Singleton for now; maybe eventually support multiplayer
        static const User<EffectType, OtherDataTypes...> current_user;

    private:
        const LeftHand<EffectType, OtherDataTypes...> leftHand;
        const RightHand<EffectType, OtherDataTypes...> rightHand;
        User();
    };

    template <typename EffectType, typename... OtherDataTypes>
    User<EffectType, OtherDataTypes...>::User()
        : leftHand(LeftHand<EffectType, OtherDataTypes...>()),
          rightHand(RightHand<EffectType, OtherDataTypes...>()) { }

    template <typename EffectType, typename... OtherDataTypes>
    const User<EffectType, OtherDataTypes...> User<EffectType, OtherDataTypes...>::current_user =
            User<EffectType, OtherDataTypes...>();
}
