//
// Created by zach_contactci on 4/26/2023.
//

#pragma once

#include <cci_lib_defs.h>

#include <stdint.h>

typedef struct _DeviceConnectivityStatus {
    bool isRight;
    bool isConnected;
} DeviceConnectivityStatus;
/**
@defgroup gloveManagement Glove management functions
@brief These functions are used to manage and reference the Maestro gloves.

<strong>`#include "maestro.h"`</strong>

`start_maestro_detection_service()` must be called before calibration, haptics, or any motion capture data can be used.
The returned pointers should be stored and passed into all other Maestro API functions.
 */

/* @{ */
/**
 * Start the Maestro glove detection thread. This function must be called in order for the Maestro glove(s) to connect.
 * This will spawn a new thread that manages the connecting and disconnecting of Maestro gloves.
 * @return Whether or not the detection thread started successfully.
 */
CCI_API_FUNC(bool) start_maestro_detection_service();

/**
 * Stop the Maestro glove detection thread and free resources.
 * @return Whether or not the detection thread was stopped.
 */
CCI_API_FUNC(bool) stop_maestro_detection_service();

/**
 * @return The pointer to the left glove context. This value should be passed into other function that set or retrieve values for the left glove.
 */
CCI_API_FUNC(intptr_t const) get_left_glove_pointer();

/**
 * @return The pointer to the right glove context. This value should be passed into other functions that set or retrieve values for the right glove.
 */
CCI_API_FUNC(intptr_t const) get_right_glove_pointer();

/* @} */

/**
@defgroup hapticsControl Haptic control functions
@brief These functions are used to control the vibration and force-feedback haptics for an individual finger.
 */

/**
@defgroup vibrationControl Vibration control functions
@ingroup hapticsControl
@brief These functions are used to set the vibration effect for an individual finger.

<strong>`#include "maestro.h"`</strong>

<a href="https://www.precisionmicrodrives.com/tech-blog/2015/12/16/which-haptic-effects-should-you-use">
 Here's a good article if you're unsure which effect you should use for certain interactions.</a>

Below is a list of available vibration effects (the DRV2605 haptic driver is used):
 <img src="../images/vibration_effects.png">
 */

CCI_API_FUNC(void) start_haptic_transaction(intptr_t maestroPtr);
CCI_API_FUNC(void) end_haptic_transaction(intptr_t maestroPtr);

/* @{ */

/**
 * Sets the vibration effect for the thumb.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param effect The vibration effect to be used, or `0` for no vibration.
 * @param modifier Modifiers to be applied to vibration. 0 for default.
 */
CCI_API_FUNC(void) set_thumb_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier);

/**
 * Sets the vibration effect for the index finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param effect The vibration effect to be used, or `0` for no vibration.
 * @param modifier Modifiers to be applied to vibration. 0 for default.
 */
CCI_API_FUNC(void) set_index_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier);

/**
 * Sets the vibration effect for the middle finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param effect The vibration effect to be used, or `0` for no vibration.
 * @param modifier Modifiers to be applied to vibration. 0 for default.
 */
CCI_API_FUNC(void) set_middle_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier);

/**
 * Sets the vibration effect for the ring finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param effect The vibration effect to be used, or `0` for no vibration.
 * @param modifier Modifiers to be applied to vibration. 0 for default.
 */
CCI_API_FUNC(void) set_ring_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier);

/**
 * Sets the vibration effect for the little (pinky) finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param effect The vibration effect to be used, or `0` for no vibration.
 * @param modifier Modifiers to be applied to vibration. 0 for default.
 */
CCI_API_FUNC(void) set_little_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier);

/**
 * Sets the vibration amplitude for the thumb.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param amplitude The vibration amplitude to be used, scalar from 0 to 1.
 */
CCI_API_FUNC(void) set_thumb_vibration_amplitude(intptr_t maestroPtr, float amplitude);

/**
 * Sets the vibration amplitude for the index finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param amplitude The vibration amplitude to be used, scalar from 0 to 1.
 */
CCI_API_FUNC(void) set_index_vibration_amplitude(intptr_t maestroPtr, float amplitude);

/**
 * Sets the vibration amplitude for the middle finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param amplitude The vibration amplitude to be used, scalar from 0 to 1.
 */
CCI_API_FUNC(void) set_middle_vibration_amplitude(intptr_t maestroPtr, float amplitude);

/**
 * Sets the vibration amplitude for the ring finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param amplitude The vibration amplitude to be used, scalar from 0 to 1.
 */
CCI_API_FUNC(void) set_ring_vibration_amplitude(intptr_t maestroPtr, float amplitude);

/**
 * Sets the vibration amplitude for the little (pinky) finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param amplitude The vibration amplitude to be used, scalar from 0 to 1.
 */
CCI_API_FUNC(void) set_little_vibration_amplitude(intptr_t maestroPtr, float amplitude);

/* @} */

/**
@defgroup forceFeedbackControl Force-feedback control functions
@ingroup hapticsControl
@brief These functions are used to set the force-feedback motor amplitude for an individual finger.

 <strong>`#include "maestro.h"`</strong>
 */

/* @{ */

/**
 * Sets the force-feedback motor amplitude for the thumb.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param amplitude The new amplitude for the force-feedback motor.
 */
CCI_API_FUNC(void) set_thumb_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude);

/**
 * Sets the force-feedback motor amplitude for the index finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param amplitude The new amplitude for the force-feedback motor.
 */
CCI_API_FUNC(void) set_index_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude);

/**
 * Sets the force-feedback motor amplitude for the middle finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param amplitude The new amplitude for the force-feedback motor.
 */
CCI_API_FUNC(void) set_middle_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude);

/**
 * Sets the force-feedback motor amplitude for the ring finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param amplitude The new amplitude for the force-feedback motor.
 */
CCI_API_FUNC(void) set_ring_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude);

/**
 * Sets the force-feedback motor amplitude for the little (pinky) finger.
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param amplitude The new amplitude for the force-feedback motor.
 */
CCI_API_FUNC(void) set_little_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude);

/* @} */
