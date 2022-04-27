//
// Created by john_contactci on 3/7/2022.
//

#ifndef CONTACTCI_EFFECT_H
#define CONTACTCI_EFFECT_H

#include <stdint.h>

#include "core.h"
#include "hand.h"

typedef void* VibrationAtomHandle;
typedef void* ForceFeedbackAtomHandle;

typedef void* FrameHandle;

typedef void* EffectHandle;
typedef void* EffectBuilderHandle;

LIBRARY_API VibrationAtomHandle cci_atom_vibration_create(uint8_t effect);
LIBRARY_API void cci_atom_vibration_destroy(VibrationAtomHandle atom_handle);

LIBRARY_API ForceFeedbackAtomHandle cci_atom_force_feedback_create(float amplitude);
LIBRARY_API void cci_atom_force_feedback_destroy(ForceFeedbackAtomHandle atom_handle);

LIBRARY_API FrameHandle cci_frame_create(VibrationAtomHandle vibration, ForceFeedbackAtomHandle force_feedback);
LIBRARY_API void cci_frame_destroy(FrameHandle frame_handle);

// TODO take Frame instead?
LIBRARY_API EffectHandle cci_effect_create(VibrationAtomHandle initial_vibration, ForceFeedbackAtomHandle initial_force_feedback);
LIBRARY_API void cci_effect_destroy(EffectHandle effect_handle);

LIBRARY_API EffectBuilderHandle cci_effect_builder_create(EffectHandle initial_effect);
LIBRARY_API void cci_effect_builder_destroy(EffectBuilderHandle effect_builder_handle);

LIBRARY_API void cci_effect_builder_scale(EffectBuilderHandle effect_builder_handle, double scalar);
LIBRARY_API void cci_effect_builder_delay(EffectBuilderHandle effect_builder_handle, uint32_t length);
LIBRARY_API void cci_effect_builder_sleep(EffectBuilderHandle effect_builder_handle, uint32_t length);

LIBRARY_API void cci_effect_builder_then(EffectBuilderHandle effect_builder_handle, EffectHandle other_effect);
LIBRARY_API void cci_effect_builder_repeat(EffectBuilderHandle effect_builder_handle, uint32_t times);

// TODO not exporting `join` since it doesn't make sense when there are no templates involved

LIBRARY_API void cci_effect_builder_interpolate_ramp(
    EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames
);
LIBRARY_API void cci_effect_builder_interpolate_ease_out(
    EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames
);
LIBRARY_API void cci_effect_builder_interpolate_ease_in(
    EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames
);
LIBRARY_API void cci_effect_builder_interpolate_ease_in_out(
    EffectBuilderHandle effect_builder_handle, FrameHandle to_frame, uint32_t over_frames
);

LIBRARY_API void cci_effect_builder_dampen_ramp(EffectBuilderHandle effect_builder_handle, uint32_t over_frames);
LIBRARY_API void cci_effect_builder_dampen_ease_out(EffectBuilderHandle effect_builder_handle, uint32_t over_frames);
LIBRARY_API void cci_effect_builder_dampen_ease_in(EffectBuilderHandle effect_builder_handle, uint32_t over_frames);
LIBRARY_API void cci_effect_builder_dampen_ease_in_out(EffectBuilderHandle effect_builder_handle, uint32_t over_frames);

LIBRARY_API void cci_effect_builder_build(EffectBuilderHandle effect_builder_handle, EffectHandle effect_out);

LIBRARY_API void cci_apply_effect(EffectHandle effect, Handedness handedness, HandTreeIndexHandle hand_part);

#endif //CONTACTCI_EFFECT_H
