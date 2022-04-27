//
// Created by john_contactci on 3/9/2022.
//

#include "effect.h"
#include "hand.h"
#include "core.h"

#include "core/haptics/effects/effect.h"
#include "core/haptics/effects/builder.h"
#include "core/hands/hand_tree.h"
#include "core/user.h"
#include "../../lib-impl/include/atoms.h"

using namespace contactci::core::haptics::effects;
using namespace contactci::core::haptics::atoms;
using namespace contactci::core::hands;
using namespace contactci::core;

template <typename AtomType>
static inline AtomType cast_atom_handle(void *generic_atom) {
    return generic_atom == nullptr
            ? AtomType::get_zero()
            : *reinterpret_cast<AtomType*>(generic_atom);
}

VibrationAtomHandle cci_atom_vibration_create(uint8_t effect) {
    return reinterpret_cast<VibrationAtomHandle>(new VibrationAtom(effect));
}

void cci_atom_vibration_destroy(VibrationAtomHandle atom_handle) {
    delete reinterpret_cast<VibrationAtom*>(atom_handle);
}

ForceFeedbackAtomHandle cci_atom_force_feedback_create(float amplitude) {
    return reinterpret_cast<ForceFeedbackAtomHandle>(new ForceFeedbackAtom(amplitude));
}

void cci_atom_force_feedback_destroy(ForceFeedbackAtomHandle atom_handle) {
    delete reinterpret_cast<ForceFeedbackAtom*>(atom_handle);
}

FrameHandle cci_frame_create(VibrationAtomHandle vibration, ForceFeedbackAtomHandle force_feedback) {
    auto vibrationAtom = cast_atom_handle<VibrationAtom>(vibration);
    auto forceFeedbackAtom = cast_atom_handle<ForceFeedbackAtom>(force_feedback);

    return reinterpret_cast<FrameHandle>(
        new Frame<VibrationAtom, ForceFeedbackAtom>(vibrationAtom, forceFeedbackAtom)
    );
}

void cci_frame_destroy(FrameHandle frame_handle) {
    delete reinterpret_cast<Frame<VibrationAtom, ForceFeedbackAtom>*>(frame_handle);
}

// TODO take Frame instead?
EffectHandle cci_effect_create(VibrationAtomHandle initial_vibration, ForceFeedbackAtomHandle initial_force_feedback) {
    auto vibrationAtom = cast_atom_handle<VibrationAtom>(initial_vibration);
    auto forceFeedbackAtom = cast_atom_handle<ForceFeedbackAtom>(initial_force_feedback);

    return reinterpret_cast<EffectHandle>(
        new Effect<VibrationAtom, ForceFeedbackAtom>(vibrationAtom, forceFeedbackAtom)
    );
}

void cci_effect_destroy(EffectHandle effect_handle) {
    delete reinterpret_cast<Effect<VibrationAtom, ForceFeedbackAtom>*>(effect_handle);
}

EffectBuilderHandle cci_effect_builder_create(EffectHandle initial_effect) {
    auto *effect = reinterpret_cast<Effect<VibrationAtom, ForceFeedbackAtom>*>(initial_effect);

    return reinterpret_cast<EffectBuilderHandle>(
        new EffectBuilder<VibrationAtom, ForceFeedbackAtom>(*effect)
    );
}

void cci_effect_builder_destroy(EffectBuilderHandle effect_builder_handle) {
    delete reinterpret_cast<EffectBuilder<VibrationAtom, ForceFeedbackAtom>*>(effect_builder_handle);
}

void cci_effect_builder_scale(EffectBuilderHandle effect_builder_handle, double scalar) {
    auto *builder = reinterpret_cast<EffectBuilder<VibrationAtom, ForceFeedbackAtom>*>(effect_builder_handle);

    builder->scale(scalar);
}

void cci_effect_builder_delay(EffectBuilderHandle effect_builder_handle, uint32_t length) {
    auto *builder = reinterpret_cast<EffectBuilder<VibrationAtom, ForceFeedbackAtom>*>(effect_builder_handle);

    builder->delay(length);
}

void cci_effect_builder_sleep(EffectBuilderHandle effect_builder_handle, uint32_t length) {
    auto *builder = reinterpret_cast<EffectBuilder<VibrationAtom, ForceFeedbackAtom>*>(effect_builder_handle);

    builder->sleep(length);
}

void cci_effect_builder_then(EffectBuilderHandle effect_builder_handle, EffectHandle other_effect) {
    auto *builder = reinterpret_cast<EffectBuilder<VibrationAtom, ForceFeedbackAtom>*>(effect_builder_handle);
    auto *effect = reinterpret_cast<Effect<VibrationAtom, ForceFeedbackAtom>*>(other_effect);

    builder->then(*effect);
}

void cci_effect_builder_repeat(EffectBuilderHandle effect_builder_handle, uint32_t times) {
    auto *builder = reinterpret_cast<EffectBuilder<VibrationAtom, ForceFeedbackAtom>*>(effect_builder_handle);

    builder->repeat(times);
}

// TODO not exporting `join` since it doesn't make sense when there are no templates involved

template <typename DistortFunc>
static inline void cci_effect_builder_interpolate(
        EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames, DistortFunc &&func) {
    auto *builder = reinterpret_cast<EffectBuilder<VibrationAtom, ForceFeedbackAtom>*>(effect_builder_handle);
    auto *frame = reinterpret_cast<Frame<VibrationAtom, ForceFeedbackAtom>*>(to_frame);

    builder->interpolate(*frame, over_frames, func);
}

void cci_effect_builder_interpolate_ramp(
        EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames) {
    cci_effect_builder_interpolate(effect_builder_handle, to_frame, over_frames, interpolation::ramp);
}

void cci_effect_builder_interpolate_ease_out(
        EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames) {
    cci_effect_builder_interpolate(effect_builder_handle, to_frame, over_frames, interpolation::ease_out);
}

void cci_effect_builder_interpolate_ease_in(
        EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames) {
    cci_effect_builder_interpolate(effect_builder_handle, to_frame, over_frames, interpolation::ease_in);
}

void cci_effect_builder_interpolate_ease_in_out(
        EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames) {
    cci_effect_builder_interpolate(effect_builder_handle, to_frame, over_frames, interpolation::ease_in_out);
}

template <typename DistortFunc>
static inline void cci_effect_builder_dampen(
        EffectBuilderHandle effect_builder_handle, uint32_t over_frames, DistortFunc &&func) {
    auto *builder = reinterpret_cast<EffectBuilder<VibrationAtom, ForceFeedbackAtom>*>(effect_builder_handle);

    builder->template dampen(over_frames, func);
}

void cci_effect_builder_dampen_ramp(EffectBuilderHandle effect_builder_handle, uint32_t over_frames) {
    cci_effect_builder_dampen(effect_builder_handle, over_frames, interpolation::ramp);
}

void cci_effect_builder_dampen_ease_out(EffectBuilderHandle effect_builder_handle, uint32_t over_frames) {
    cci_effect_builder_dampen(effect_builder_handle, over_frames, interpolation::ease_out);
}

void cci_effect_builder_dampen_ease_in(EffectBuilderHandle effect_builder_handle, uint32_t over_frames) {
    cci_effect_builder_dampen(effect_builder_handle, over_frames, interpolation::ease_in);
}

void cci_effect_builder_dampen_ease_in_out(EffectBuilderHandle effect_builder_handle, uint32_t over_frames) {
    cci_effect_builder_dampen(effect_builder_handle, over_frames, interpolation::ease_in_out);
}

void cci_effect_builder_build(EffectBuilderHandle effect_builder_handle, EffectHandle effect_out) {
    auto *builder = reinterpret_cast<EffectBuilder<VibrationAtom, ForceFeedbackAtom>*>(effect_builder_handle);
    auto *effect = reinterpret_cast<Effect<VibrationAtom, ForceFeedbackAtom>*>(effect_out);

    *effect = builder->build();
}

static auto delete_on_effect_completed = [](EffectHandle effect) {
    delete reinterpret_cast<Effect<VibrationAtom, ForceFeedbackAtom>*>(effect);
};

void cci_apply_effect(
    EffectHandle effect,
    Handedness handedness,
    HandTreeIndexHandle hand_part,
    void (*on_effect_completed)(EffectHandle effect) = delete_on_effect_completed
) {
    auto &user = User<VibrationAtom, ForceFeedbackAtom>::current_user;
    WhichHand which_hand = handedness == Left ? WhichHand::Left : WhichHand::Right;
    auto *typed_effect = reinterpret_cast<Effect<VibrationAtom, ForceFeedbackAtom>*>(effect);
    auto *hand_tree_index_ptr = reinterpret_cast<const HandTreeIndex*>(hand_part);
    auto *typed_on_effect_completed = reinterpret_cast<void (*)(Effect<VibrationAtom, ForceFeedbackAtom> *)>(on_effect_completed);

    user.apply_effect(which_hand, *hand_tree_index_ptr, *typed_effect, typed_on_effect_completed);
}

void cci_update(CommunicatorHandle comms) {
    auto &user = User<VibrationAtom, ForceFeedbackAtom>::current_user;
    auto *comms_ptr = reinterpret_cast<Communicator*>(comms);
    //TODO we're dereferencing a pointer that could be bad
    user.update(*comms_ptr);
}

const struct _HandConstants HandConstants = {
    .FINGER_ROOT = reinterpret_cast<HandTreeIndexHandle>(&constants::FINGER_ROOT),

    .THUMB_FINGER_METACARPAL = reinterpret_cast<HandTreeIndexHandle>(&constants::THUMB_FINGER_METACARPAL),
    .THUMB_FINGER_PROXIMAL = reinterpret_cast<HandTreeIndexHandle>(&constants::THUMB_FINGER_PROXIMAL),
    /* Thumb has no middle segment */
    .THUMB_FINGER_DISTAL = reinterpret_cast<HandTreeIndexHandle>(&constants::THUMB_FINGER_DISTAL),

    .INDEX_FINGER_METACARPAL = reinterpret_cast<HandTreeIndexHandle>(&constants::INDEX_FINGER_METACARPAL),
    .INDEX_FINGER_PROXIMAL = reinterpret_cast<HandTreeIndexHandle>(&constants::INDEX_FINGER_PROXIMAL),
    .INDEX_FINGER_MIDDLE = reinterpret_cast<HandTreeIndexHandle>(&constants::INDEX_FINGER_MIDDLE),
    .INDEX_FINGER_DISTAL = reinterpret_cast<HandTreeIndexHandle>(&constants::INDEX_FINGER_DISTAL),

    .MIDDLE_FINGER_METACARPAL = reinterpret_cast<HandTreeIndexHandle>(&constants::MIDDLE_FINGER_METACARPAL),
    .MIDDLE_FINGER_PROXIMAL = reinterpret_cast<HandTreeIndexHandle>(&constants::MIDDLE_FINGER_PROXIMAL),
    .MIDDLE_FINGER_MIDDLE = reinterpret_cast<HandTreeIndexHandle>(&constants::MIDDLE_FINGER_MIDDLE),
    .MIDDLE_FINGER_DISTAL = reinterpret_cast<HandTreeIndexHandle>(&constants::MIDDLE_FINGER_DISTAL),

    .RING_FINGER_METACARPAL = reinterpret_cast<HandTreeIndexHandle>(&constants::RING_FINGER_METACARPAL),
    .RING_FINGER_PROXIMAL = reinterpret_cast<HandTreeIndexHandle>(&constants::RING_FINGER_PROXIMAL),
    .RING_FINGER_MIDDLE = reinterpret_cast<HandTreeIndexHandle>(&constants::RING_FINGER_MIDDLE),
    .RING_FINGER_DISTAL = reinterpret_cast<HandTreeIndexHandle>(&constants::RING_FINGER_DISTAL),

    .LITTLE_FINGER_METACARPAL = reinterpret_cast<HandTreeIndexHandle>(&constants::LITTLE_FINGER_METACARPAL),
    .LITTLE_FINGER_PROXIMAL = reinterpret_cast<HandTreeIndexHandle>(&constants::LITTLE_FINGER_PROXIMAL),
    .LITTLE_FINGER_MIDDLE = reinterpret_cast<HandTreeIndexHandle>(&constants::LITTLE_FINGER_MIDDLE),
    .LITTLE_FINGER_DISTAL = reinterpret_cast<HandTreeIndexHandle>(&constants::LITTLE_FINGER_DISTAL)
};
