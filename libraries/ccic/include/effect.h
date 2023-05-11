//
// Created by john_contactci on 3/7/2022.
//

#ifndef CONTACTCI_EFFECT_H
#define CONTACTCI_EFFECT_H

#include <cci_lib_defs.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "hand.h"

typedef void* VibrationAtomHandle;
typedef void* ForceFeedbackAtomHandle;

typedef void* FrameHandle;

typedef void* EffectHandle;
typedef void* EffectBuilderHandle;

/*
CCI_API_FUNC(VibrationAtomHandle) cci_atom_vibration_create(uint8_t effect);
CCI_API_FUNC(void) cci_atom_vibration_destroy(VibrationAtomHandle atom_handle);

CCI_API_FUNC(ForceFeedbackAtomHandle) cci_atom_force_feedback_create(float amplitude);
CCI_API_FUNC(void) cci_atom_force_feedback_destroy(ForceFeedbackAtomHandle atom_handle);

CCI_API_FUNC(FrameHandle) cci_frame_create(VibrationAtomHandle vibration, ForceFeedbackAtomHandle force_feedback);
CCI_API_FUNC(void) cci_frame_destroy(FrameHandle frame_handle);

// TODO take Frame instead?
CCI_API_FUNC(EffectHandle) cci_effect_create_empty();
CCI_API_FUNC(EffectHandle) cci_effect_create(VibrationAtomHandle initial_vibration, ForceFeedbackAtomHandle initial_force_feedback);
CCI_API_FUNC(void) cci_effect_destroy(EffectHandle effect_handle);

CCI_API_FUNC(EffectBuilderHandle) cci_effect_builder_create(EffectHandle initial_effect);
CCI_API_FUNC(void) cci_effect_builder_destroy(EffectBuilderHandle effect_builder_handle);

CCI_API_FUNC(void) cci_effect_builder_scale(EffectBuilderHandle effect_builder_handle, double scalar);
CCI_API_FUNC(void) cci_effect_builder_delay(EffectBuilderHandle effect_builder_handle, uint32_t length);
CCI_API_FUNC(void) cci_effect_builder_sleep(EffectBuilderHandle effect_builder_handle, uint32_t length);

CCI_API_FUNC(void) cci_effect_builder_then(EffectBuilderHandle effect_builder_handle, EffectHandle other_effect);
CCI_API_FUNC(void) cci_effect_builder_repeat(EffectBuilderHandle effect_builder_handle, uint32_t times);

// TODO not exporting `join` since it doesn't make sense when there are no templates involved

CCI_API_FUNC(void) cci_effect_builder_interpolate_ramp(
    EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames
);
CCI_API_FUNC(void) cci_effect_builder_interpolate_ease_out(
    EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames
);
CCI_API_FUNC(void) cci_effect_builder_interpolate_ease_in(
    EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames
);
CCI_API_FUNC(void) cci_effect_builder_interpolate_ease_in_out(
    EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames
);

CCI_API_FUNC(void) cci_effect_builder_dampen_ramp(EffectBuilderHandle effect_builder_handle, uint32_t over_frames);
CCI_API_FUNC(void) cci_effect_builder_dampen_ease_out(EffectBuilderHandle effect_builder_handle, uint32_t over_frames);
CCI_API_FUNC(void) cci_effect_builder_dampen_ease_in(EffectBuilderHandle effect_builder_handle, uint32_t over_frames);
CCI_API_FUNC(void) cci_effect_builder_dampen_ease_in_out(EffectBuilderHandle effect_builder_handle, uint32_t over_frames);

CCI_API_FUNC(void) cci_effect_builder_build(EffectBuilderHandle effect_builder_handle, EffectHandle effect_out);

CCI_API_FUNC(void) cci_apply_effect(EffectHandle effect, enum Handedness handedness, HandTreeIndexHandle hand_part, void (*on_effect_completed)(EffectHandle));
*/

#endif //CONTACTCI_EFFECT_H
