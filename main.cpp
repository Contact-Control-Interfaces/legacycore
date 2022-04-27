//
// Created by john_contactci on 2/3/2022.
//

#include <core/user.h>
#include <core/haptics/effects/effect.h>
#include <core/haptics/effects/builder.h>
#include <core/haptics/effects/player.h>

#include <comms/communicator.h>

#include "lib-impl/include/atoms.h"

#include <iostream>
#include <cstdint>

using namespace contactci::core;
using namespace contactci::core::haptics;
using namespace contactci::core::haptics::effects;
using namespace contactci::core::haptics::atoms;

class DebugCommunicator : public contactci::comms::Communicator {
public:
    void start_frame() override {
        /* TODO */
    }

    void end_frame() override {
        std::cout << std::endl;
    }

    void send(uint8_t byte) override {
        std::cout << (int)byte << '\t';
    }
};

class ChainTest {
public:
    ChainTest() {
        std::cout << "construct ChainTest " << this << std::endl;
    }

    ~ChainTest() {
        std::cout << "destroy ChainTest " << this << std::endl;
    }

    std::shared_ptr<ChainTest> chain() {
        return std::make_shared<ChainTest>();
    }
};

void asdf(ChainTest &chain);

int main() {
    auto vs = VibrationAtom(52);
    auto fs = ForceFeedbackAtom(175);

    Effect<ForceFeedbackAtom> asdf(ForceFeedbackAtom(0));

    Effect<VibrationAtom, ForceFeedbackAtom> temp3(vs, fs);

    DebugCommunicator comms;

    // TODO returning Effect by value is super expensive
    // We should figure out how to return references
    // Probably requires alloc'ing on the heap, so need to ensure we don't leak somehow
    // Chaining would cause leaks
//
//    auto asdf2 =
//            Effect<ForceFeedbackAtom>(ForceFeedbackAtom(0))
//                    .repeat(5)                                      // count 1
//                    .then(ForceFeedbackAtom(1.0f));    // count 1

EffectBuilder<ForceFeedbackAtom>(ForceFeedbackAtom(0))
        .repeat(5)
        .then(ForceFeedbackAtom(1.0f));

    auto asdf3 =
            EffectBuilder<ForceFeedbackAtom>(ForceFeedbackAtom(0))
                    .repeat(5)
                    .then(ForceFeedbackAtom(1.0f))
            .map([](ForceFeedbackAtom atom) {
                if (atom != ForceFeedbackAtom::get_zero()) {
                    return atom;
                }

                return ForceFeedbackAtom(0.5f);
            })
            .sleep(3)
            .then(ForceFeedbackAtom(0.5f))
        .join(EffectBuilder<VibrationAtom>(VibrationAtom(52))
            .delay(5).build()
        ).build();
    //auto fdaf = EffectBuilder<ForceFeedbackAtom>(ForceFeedbackAtom(1.0)).dampen(10, interpolation::ease_in_out).build();

    auto &user = User<ForceFeedbackAtom, VibrationAtom>::current_user;
    user.apply_effect(contactci::core::hands::WhichHand::Right, contactci::core::constants::INDEX_FINGER_DISTAL, asdf3, [](Effect<ForceFeedbackAtom, VibrationAtom> *effect){
        std::cout << "Playing finished" << std::endl;
    });

    for (int i = 0; i < 10; i++)
        user.update(comms);
//
//    auto index = HandTreeIndex::CONSTANTS.INDEX_FINGER_DISTAL;
//
//    user.rightHand.set_value_at<Effect<ForceFeedbackAtom>>(index, fdaf);
//
//    UserEffectPlayer::play(comms, user);

    return 0;
}
