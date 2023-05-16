//
// Created by zach_contactci on 4/26/2023.
//

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief A structure containing the force-feedback motor amplitudes for each finger.
 *
 * These are the amplitudes used to control the force-feedback motors for each finger. They are proportional to the
 * power output by the motor, where `255` is the maximum force and `0` is no force.
 * See `get_force_feedback_context()`.
 */
typedef struct {
    /**
     * @brief The force-feedback motor amplitude for the thumb.
     */
    uint8_t thumb_motor_amplitude;

    /**
     * @brief The force-feedback motor amplitude for the index finger.
     */
    uint8_t index_motor_amplitude;

    /**
     * @brief The force-feedback motor amplitude for the middle finger.
     */
    uint8_t middle_motor_amplitude;

    /**
     * @brief The force-feedback motor amplitude for the ring finger.
     */
    uint8_t ring_motor_amplitude;

    /**
     * @brief The force-feedback motor amplitude for the little finger.
     */
    uint8_t little_motor_amplitude;
} ForceFeedbackContext;

/**
 * @brief A structure containing the vibration effects for each finger.
 *
 * These are the vibration effects associated with each finger. An effect of `0` means no vibration.
 * A full list of vibration effects can be found on the main page of the documentation, and also on the
 * \htmlonly<a href="group__vibration_control.html">Vibration control module</a>\endhtmlonly page.
 * See `get_vibration_context()`.
 */
typedef struct {
    /**
     * @brief The current vibration effect for the thumb.
     */
    uint8_t thumb_vibration_effect;

    /**
     * @brief The current vibration effect for the index finger.
     */
    uint8_t index_vibration_effect;

    /**
     * @brief The current vibration effect for the middle finger.
     */
    uint8_t middle_vibration_effect;

    /**
     * @brief The current vibration effect for the ring finger.
     */
    uint8_t ring_vibration_effect;

    /**
     * @brief The current vibration effect for the little finger.
     */
    uint8_t little_vibration_effect;
} VibrationContext;

typedef struct{
    /**
     * @brief The current vibration modifier for the thumb.
     */
    uint8_t thumb_vibration_modifier;
    /**
     * @brief The current vibration modifier for the index finger.
     */
    uint8_t index_vibration_modifier;
    /**
     * @brief The current vibration modifier for the middle finger.
     */
    uint8_t middle_vibration_modifier;
    /**
     * @brief The current vibration modifier for the ring finger.
     */
    uint8_t ring_vibration_modifier;
    /**
     * @brief The current vibration modifier for the little finger.
     */
    uint8_t little_vibration_modifier;
} VibrationModifierContext;

/**
 * @brief A structure containing vibration, force feedback, raw displacement data, and whether the glove is connected.
 *
 * See `DisplacementContext`, `VibrationContext`, and `VibrationContext`.
 */
typedef struct {
    /**
     * @brief The vibration context for the glove.
     */
    volatile VibrationContext vibrationContext;

    /**
     * @brief The force feedback context for the glove.
     */
    volatile ForceFeedbackContext forceFeedbackContext;

    /**
     * @brief The vibration modifiers for the glove.
     */
    volatile VibrationModifierContext vibrationModifiers;

    /**
     * @brief Whether or not the glove is running. This is the same value returned by `is_glove_connected()`.
     */
    bool is_running;

    bool ble_mode;

    bool needs_update;
} MaestroGloveContext;

typedef void(*log_callback)(const char*, size_t length);
