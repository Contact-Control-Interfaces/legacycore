//
// Created by john_contactci on 1/31/2022.
//

#include "core/user.h"

using namespace contactci::core;

User::User() : leftHand(LeftHand()), rightHand(RightHand()) { }
User::~User() { }

const User User::current_user = User();