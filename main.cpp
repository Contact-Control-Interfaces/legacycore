//
// Created by john_contactci on 2/3/2022.
//

//#include <core/user.h>
//#include <core/haptics/effects/effect.h>
//#include <core/haptics/effects/builder.h>
//#include <core/haptics/effects/player.h>

//#include <atoms.h>

#include <effect.h>

#include <comms/communicator.h>

#include <iostream>
#include <cstdint>

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

int main() {
//    auto vs = VibrationAtom(52);
//    auto fs = ForceFeedbackAtom(175);
//
//    Effect<ForceFeedbackAtom> asdf(ForceFeedbackAtom(0));
//
//    Effect<VibrationAtom, ForceFeedbackAtom> temp3(vs, fs);

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

//EffectBuilder<ForceFeedbackAtom>(ForceFeedbackAtom(0))
//        .repeat(5)
//        .then(ForceFeedbackAtom(1.0f));
//
//    auto asdf3 =
//            EffectBuilder<ForceFeedbackAtom>(ForceFeedbackAtom(0))
//                    .repeat(5)
//                    .then(ForceFeedbackAtom(1.0f))
//            .map([](ForceFeedbackAtom atom) {
//                if (atom != ForceFeedbackAtom::get_zero()) {
//                    return atom;
//                }
//
//                return ForceFeedbackAtom(0.5f);
//            })
//            .sleep(3)
//            .then(ForceFeedbackAtom(0.5f))
//        .join(EffectBuilder<VibrationAtom>(VibrationAtom(52))
//            .delay(5).build()
//        ).build();
//    //auto fdaf = EffectBuilder<ForceFeedbackAtom>(ForceFeedbackAtom(1.0)).dampen(10, interpolation::ease_in_out).build();
//
//    auto &user = User<ForceFeedbackAtom, VibrationAtom>::current_user;
//    user.apply_effect(contactci::core::hands::WhichHand::Right, contactci::core::constants::INDEX_FINGER_DISTAL, asdf3, [](Effect<ForceFeedbackAtom, VibrationAtom> *effect){
//        std::cout << "Playing finished" << std::endl;
//    });


    VibrationAtomHandle vib_atom = cci_atom_vibration_create(52);
    ForceFeedbackAtomHandle ff_atom = cci_atom_force_feedback_create(0.5f);

    EffectHandle initial_vib_effect = cci_effect_create(vib_atom, nullptr);
    EffectHandle initial_ff_effect = cci_effect_create(nullptr, ff_atom);

    EffectHandle repeated_ff_effect = cci_effect_create_empty();

    EffectBuilderHandle repeat_ff_builder = cci_effect_builder_create(initial_ff_effect);

    cci_effect_builder_repeat(repeat_ff_builder, 5);
    cci_effect_builder_build(repeat_ff_builder, repeated_ff_effect);

    EffectHandle effect = cci_effect_create_empty();

    EffectBuilderHandle builder = cci_effect_builder_create(initial_vib_effect);

    cci_effect_builder_repeat(builder, 5);

    cci_effect_builder_then(builder, repeated_ff_effect);

    cci_effect_builder_build(builder, effect);

    cci_apply_effect(effect, Right, HandConstants.INDEX_FINGER_DISTAL, nullptr);

    auto *comms_guh = reinterpret_cast<CommunicatorHandle>(&comms);

    for (int i = 0; i < 10; i++)
        cci_update(comms_guh);
//
//    auto index = HandTreeIndex::CONSTANTS.INDEX_FINGER_DISTAL;
//
//    user.rightHand.set_value_at<Effect<ForceFeedbackAtom>>(index, fdaf);
//
//    UserEffectPlayer::play(comms, user);

    return 0;
}
