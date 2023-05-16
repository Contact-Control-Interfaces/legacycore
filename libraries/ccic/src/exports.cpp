//
// Created by john_contactci on 3/9/2022.
//

#include "effect.h"
#include "hand.h"
#include "core.h"

#include "haptic_types.h"
#include "haptics.h"

#include <core/haptics/effects/effect.h>
#include <core/haptics/effects/builder.h>
#include <core/hands/hand_tree.h>
#include <core/user.h>

#include <atoms.h>
#include <channel.h>

using namespace contactci::core::haptics::effects;
using namespace contactci::core::haptics::atoms;
using namespace contactci::core::hands;
using namespace contactci::core;

using namespace contactci::io;

// File scope for now
// TODO make sure to free this
contactci::io::PipeChannel* hapticsChannel;
log_callback logger = nullptr;


const uint32_t ThumbMask =    1 << 0;
const uint32_t IndexMask =    1 << 1;
const uint32_t MiddleMask =   1 << 2;
const uint32_t RingMask =     1 << 3;
const uint32_t LittleMask =   1 << 4;

void UpdateForceFeedback(bool isRight, uint8_t amplitude, uint32_t bitmask);
void UpdateVibration(bool isRight, uint8_t effect, uint8_t modifiers, uint32_t bitmask);
bool IsDeviceConnected(bool isRight);
bool IsBleProcessing();

/*template <typename AtomType>
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

EffectHandle cci_effect_create_empty() {
    return reinterpret_cast<EffectHandle>(new Effect<VibrationAtom, ForceFeedbackAtom>());
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

void cci_apply_effect(
    EffectHandle effect,
    Handedness handedness,
    HandTreeIndexHandle hand_part,
    void (*on_effect_completed)(EffectHandle)
) {
    auto &user = User<VibrationAtom, ForceFeedbackAtom>::current_user;
    WhichHand which_hand = handedness == Left ? WhichHand::Left : WhichHand::Right;
    auto *typed_effect = reinterpret_cast<Effect<VibrationAtom, ForceFeedbackAtom>*>(effect);
    auto *hand_tree_index_ptr = reinterpret_cast<const HandTreeIndex*>(hand_part);
    // Default to destroying/freeing the Effect after playing
    auto *typed_on_effect_completed = reinterpret_cast<void (*)(Effect<VibrationAtom, ForceFeedbackAtom> *)>(
        on_effect_completed == nullptr ? cci_effect_destroy : on_effect_completed
    );

    user.apply_effect(which_hand, *hand_tree_index_ptr, *typed_effect, typed_on_effect_completed);
}*/

ChannelHandle cci_channel_pipe_create() {
    return new PipeChannel;
}

ChannelHandle cci_channel_stdout_create() {
    return new StdOutChannel;
}

void cci_channel_destroy(ChannelHandle channel) {
    delete reinterpret_cast<Channel*>(channel);
}

void cci_update(ChannelHandle channel) {
    auto &user = User<VibrationAtom, ForceFeedbackAtom>::current_user;
    auto *channel_ptr = reinterpret_cast<Channel*>(channel);
    //TODO we're dereferencing a pointer that could be bad
    user.update(*channel_ptr);
}

void cci_channel_send_set_dimension_message(ChannelHandle channel, unsigned int dimension, unsigned int flags, unsigned int bitmask, const char* values, int valuesCount){
    auto *channel_ptr = reinterpret_cast<Channel*>(channel);
    channel_ptr->send_set_dimension_message(dimension, flags, bitmask, std::string(values, valuesCount));
}

const struct HandConstants_ HandConstants = {
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

struct HandConstants_ cci_hand_constants_get() {
    return HandConstants;
}

void UpdateForceFeedback(bool isRight, uint8_t amplitude, uint32_t bitmask) {
    if (hapticsChannel != nullptr) {
        float amp = (float)amplitude / 0xFF;
        hapticsChannel->send_force_feedback_update_message(isRight, amp, bitmask);
    }
}

void UpdateVibration(bool isRight, uint8_t effect, uint8_t modifiers, uint32_t bitmask) {
    if (hapticsChannel != nullptr)
        hapticsChannel->send_vibration_update_message(isRight, effect, modifiers, bitmask);
}

bool IsDeviceConnected(bool isRight) {
    if (hapticsChannel == nullptr)
        return false;

    return hapticsChannel->check_if_device_connected(isRight);
}

bool IsBleProcessing() {
    if (hapticsChannel == nullptr)
        return false;

    return hapticsChannel->check_if_device_processing();
}

#pragma region Old C API

bool start_maestro_detection_service() {
    try {
        // See if it explodes trying to open the named pipe
        if (hapticsChannel == nullptr)
            hapticsChannel = new PipeChannel();
        return true;
    } catch (...) {
        // TODO output error?
        return false;
    }
}

bool stop_maestro_detection_service() {
    delete hapticsChannel;
    hapticsChannel = nullptr;
    return true;
}

inline intptr_t const get_left_glove_pointer() {
    return 0; // this isn't a pointer
}

inline intptr_t const get_right_glove_pointer() {
    return 1; // this isn't a pointer
}

bool is_glove_connected(intptr_t maestroPtr) {
    return IsDeviceConnected(maestroPtr == get_right_glove_pointer());
}

void set_thumb_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier) {
    UpdateVibration(maestroPtr == get_right_glove_pointer(), effect, modifier, ThumbMask);
}

void set_index_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier) {
    UpdateVibration(maestroPtr == get_right_glove_pointer(), effect, modifier, IndexMask);
}

void set_middle_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier) {
    UpdateVibration(maestroPtr == get_right_glove_pointer(), effect, modifier, MiddleMask);
}

void set_ring_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier) {
    UpdateVibration(maestroPtr == get_right_glove_pointer(), effect, modifier, RingMask);
}

void set_little_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier) {
    UpdateVibration(maestroPtr == get_right_glove_pointer(), effect, modifier, LittleMask);
}

/*
uint8_t get_thumb_vibration_effect(intptr_t maestroPtr) {

}

uint8_t get_index_vibration_effect(intptr_t maestroPtr) {

}

uint8_t get_middle_vibration_effect(intptr_t maestroPtr) {

}

uint8_t get_ring_vibration_effect(intptr_t maestroPtr) {

}

uint8_t get_little_vibration_effect(intptr_t maestroPtr) {

}*/

void set_thumb_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude) {
    UpdateForceFeedback(maestroPtr == get_right_glove_pointer(), amplitude, ThumbMask);
}

void set_index_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude) {
    UpdateForceFeedback(maestroPtr == get_right_glove_pointer(), amplitude, IndexMask);
}

void set_middle_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude) {
    UpdateForceFeedback(maestroPtr == get_right_glove_pointer(), amplitude, MiddleMask);
}

void set_ring_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude) {
    UpdateForceFeedback(maestroPtr == get_right_glove_pointer(), amplitude, RingMask);
}

void set_little_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude) {
    UpdateForceFeedback(maestroPtr == get_right_glove_pointer(), amplitude, LittleMask);
}

/*
uint8_t get_thumb_motor_amplitude(intptr_t maestroPtr) {

}

uint8_t get_index_motor_amplitude(intptr_t maestroPtr) {

}

uint8_t get_middle_motor_amplitude(intptr_t maestroPtr) {

}

uint8_t get_ring_motor_amplitude(intptr_t maestroPtr) {

}

uint8_t get_little_motor_amplitude(intptr_t maestroPtr) {

}*/

bool is_ble_processing() {
    return IsBleProcessing();
}

void install_log_callback(log_callback callback) {
    if (hapticsChannel != nullptr)
        hapticsChannel->install_log_callback(callback);
}

#pragma endregion