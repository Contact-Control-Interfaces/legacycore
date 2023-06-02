//
// Created by zach_contactci on 4/26/2023.
//

#pragma once

#include "haptic_types.h"

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

CCI_API_FUNC(DeviceConnectivityStatus) get_device_connectivity_status_update();

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Whether or not the glove referenced by `maestroPtr` is connected via bluetooth
 */
//CCI_API_FUNC(bool) is_ble_connected(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return The current Maestro data. This returns a struct containing the same values acquired by calling `get_displacement_context()`,
 * `get_vibration_context()`, `get_force_feedback_context()`, and `is_glove_connected()`.
 */
//CCI_API_FUNC(const MaestroGloveContext* const) get_glove_context(intptr_t maestroPtr);

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
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current vibration effect for the thumb.
 * @param modifier Modifiers to be applied to vibration. 0 for default.
 */
//CCI_API_FUNC(uint8_t) get_thumb_vibration_effect(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current vibration effect for the index finger.
 */
//CCI_API_FUNC(uint8_t) get_index_vibration_effect(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current vibration effect for the middle finger.
 */
//CCI_API_FUNC(uint8_t) get_middle_vibration_effect(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current vibration effect for the ring finger.
 */
//CCI_API_FUNC(uint8_t) get_ring_vibration_effect(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current vibration effect for the little (pinky) finger.
 */
//CCI_API_FUNC(uint8_t) get_little_vibration_effect(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current vibration effects. This returns a struct containing the same values acquired by `get_thumb_vibration_effect()`,
 * `get_index_vibration_effect()`, `get_middle_vibration_effect()`, `get_ring_vibration_effect()`, and `get_little_vibration_effect()`
 */
//CCI_API_FUNC(const VibrationContext* const) get_vibration_context(intptr_t maestroPtr);

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

/**
 * Sets the force-feedback PID controller targer for the thumb.
 * @param maestroPtr The pointer to the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param delta The new target of the PID controller, relative to its current position, measured in millimeters of tendon displacement. Positive displacement means the glove retracts the tendon, negative displacement means the glove lets out slack.
 */
CCI_API_FUNC(void) set_thumb_pid_target(intptr_t maestroPtr, float delta);

/**
 * Sets the force-feedback PID controller targer for the index finger.
 * @param maestroPtr The pointer to the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param delta The new target of the PID controller, relative to its current position, measured in millimeters of tendon displacement. Positive displacement means the glove retracts the ten
don, negative displacement means the glove lets out slack.
 */
CCI_API_FUNC(void) set_index_pid_target(intptr_t maestroPtr, float delta);

/**
 * Sets the force-feedback PID controller targer for the middle finger.
 * @param maestroPtr The pointer to the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param delta The new target of the PID controller, relative to its current position, measured in millimeters of tendon displacement. Positive displacement means the glove retracts the ten
don, negative displacement means the glove lets out slack.
 */
CCI_API_FUNC(void) set_middle_pid_target(intptr_t maestroPtr, float delta);

/**
 * Sets the force-feedback PID controller targer for the ring finger.
 * @param maestroPtr The pointer to the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param delta The new target of the PID controller, relative to its current position, measured in millimeters of tendon displacement. Positive displacement means the glove retracts the ten
don, negative displacement means the glove lets out slack.
 */
CCI_API_FUNC(void) set_ring_pid_target(intptr_t maestroPtr, float delta);

/**
 * Sets the force-feedback PID controller targer for the little finger.
 * @param maestroPtr The pointer to the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @param delta The new target of the PID controller, relative to its current position, measured in millimeters of tendon displacement. Positive displacement means the glove retracts the ten
don, negative displacement means the glove lets out slack.
 */
CCI_API_FUNC(void) set_little_pid_target(intptr_t maestroPtr, float delta);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current force-feedback motor amplitude for the thumb.
 */
//CCI_API_FUNC(uint8_t) get_thumb_motor_amplitude(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current force-feedback motor amplitude for the index finger.
 */
//CCI_API_FUNC(uint8_t) get_index_motor_amplitude(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current force-feedback motor amplitude for the middle finger.
 */
//CCI_API_FUNC(uint8_t) get_middle_motor_amplitude(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current force-feedback motor amplitude for the ring finger.
 */
//CCI_API_FUNC(uint8_t) get_ring_motor_amplitude(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current force-feedback motor amplitude for the little (pinky) finger.
 */
//CCI_API_FUNC(uint8_t) get_little_motor_amplitude(intptr_t maestroPtr);

/**
 * @param maestroPtr The pointer the Maestro Glove context. Obtained by get_left_glove_pointer() or get_right_glove_pointer().
 * @return Current force-feedback motor amplitudes. This returns a struct containing the same values acquired by
 * `get_thumb_motor_amplitude()`, `get_index_motor_amplitude()`, `get_middle_motor_amplitude()`,
 * `get_ring_motor_amplitude()`, and `get_little_motor_amplitude()`
 */
//CCI_API_FUNC(const ForceFeedbackContext* const) get_force_feedback_context(intptr_t maestroPtr);

/**
 * @return The maximum motor amplitude allowed.
 */
//CCI_API_FUNC(uint8_t) get_max_motor_amplitude();

/**
 * @return The default non-collision motor amplitude. It is recommended to have a motor amplitude of at least the default
 * for the sake of avoiding any slack in the force-feedback tendons.
 */
//CCI_API_FUNC(uint8_t) get_default_motor_amplitude();

/**
 * Starts up the bluetooth DLL
 * @return did Bluetooth start up successfully?
 */
//CCI_API_FUNC(bool) init_bluetooth();

/**
 * @return are we currently attempting to connect to something via BLE?
 */
CCI_API_FUNC(bool) is_ble_processing();

/**
 * @return are we currently trying to connect to a left glove?
 */
//CCI_API_FUNC(bool) is_ble_left_connecting();

/**
 * @return are we currently trying to connect to a right glove?
 */
//CCI_API_FUNC(bool) is_ble_right_connecting();

/**
 * @return are we currently scanning for BLE devices?
 */
//CCI_API_FUNC(bool) is_ble_watcher_running();

/**
 * @return number of bytes available in the serial RX buffer
 */
//CCI_API_FUNC(uint32_t) rx_bytes_available(intptr_t maestroPtr);

/**
 * Polls remote device for new data.
 * Caution! Will block calling thread for significant amounts of time!
 * @return number of bytes available in the serial RX buffer
 */
//CCI_API_FUNC(uint32_t) poll_rx(intptr_t maestroPtr);

/**
 * Reads bytes from the serial RX buffer
 * @param buffer Buffer to copy received bytes to
 * @param length Number of bytes to read
 * @return Number of bytes actually read
 */
//CCI_API_FUNC(uint32_t) read_rx_bytes(uint8_t *buffer, uint32_t length, intptr_t maestroPtr);

/**
 * Send bytes over BLE serial connection
 * @param buffer Bytes to be sent
 * @param length Number of bytes to be sent
 */
//CCI_API_FUNC(void) write_bytes(uint8_t *buffer, uint32_t length, intptr_t maestroPtr);

/**
 * Send bytes over BLE serial connection asynchronously
 * @param buffer Bytes to be sent
 * @param length Number of bytes to be sent
 */
//CCI_API_FUNC(void) write_bytes_async(uint8_t* buffer, uint32_t count, intptr_t maestroPtr);

/**
 * Sets the log callback, this function will be called
 * whenever anything needs to be logged somewhere
 * @param callback Log function to call
 */
CCI_API_FUNC(void) install_log_callback(log_callback callback);
