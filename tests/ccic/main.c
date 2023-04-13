//
// Created by john_contactci on 2/3/2022.
//

#include <core.h>
#include <effect.h>

int main() {

    //ChannelHandle channel = cci_channel_pipe_create();
    ChannelHandle channel = cci_channel_stdout_create();

    VibrationAtomHandle vib_atom = cci_atom_vibration_create(52);
    ForceFeedbackAtomHandle ff_atom = cci_atom_force_feedback_create(0.5f);

    EffectHandle initial_vib_effect = cci_effect_create(vib_atom, NULL);
    EffectHandle initial_ff_effect = cci_effect_create(NULL, ff_atom);

    EffectHandle repeated_ff_effect = cci_effect_create_empty();

    EffectBuilderHandle repeat_ff_builder = cci_effect_builder_create(initial_ff_effect);

    cci_effect_builder_repeat(repeat_ff_builder, 5);
    cci_effect_builder_build(repeat_ff_builder, repeated_ff_effect);

    EffectHandle effect = cci_effect_create_empty();

    EffectBuilderHandle builder = cci_effect_builder_create(initial_vib_effect);

    cci_effect_builder_repeat(builder, 5);

    cci_effect_builder_then(builder, repeated_ff_effect);

    cci_effect_builder_build(builder, effect);

    cci_apply_effect(effect, Right, HandConstants.INDEX_FINGER_DISTAL, NULL);

    for (int i = 0; i < 10; i++)
        cci_update(channel);

    return 0;
}
