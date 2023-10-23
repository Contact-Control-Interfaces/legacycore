//
// Created by john_contactci on 3/9/2022.
//

#include "effect.h"
#include "hand.h"
#include "core.h"

#include "haptic_types.h"
#include "haptics.h"

#include "info.h"

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

typedef struct {
    DeviceListResponseMessage responseMsg;
    DeviceDescription *deviceDescription;
} DeviceListingTransaction;

typedef struct {
    ClientListResponseMessage responseMsg;
    ClientDescription *clientDescription;
} ClientListingTransaction;

void UpdateForceFeedback(bool isRight, uint8_t amplitude, uint32_t bitmask);
void UpdateVibration(bool isRight, uint8_t effect, uint8_t modifiers, uint32_t bitmask);

DeviceDescription WrapDeviceDescriptionMessage(const DeviceDescriptionMessage& msg);
ClientDescription WrapClientDescriptionMessage(const ClientDescriptionMessage& msg);

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

DeviceDescription WrapDeviceDescriptionMessage(const DeviceDescriptionMessage& msg) {
    return DeviceDescription {
        .productLine = msg.productline().c_str(),
        .serialNumber = msg.serialnumber().c_str(),
        .isRight = msg.isright(),
        .isConnected = msg.isconnected()
    };
}

ClientDescription WrapClientDescriptionMessage(const ClientDescriptionMessage& msg) {
    return ClientDescription {
        .processID = msg.processid(),
        .processName = msg.processname().c_str()
    };
}

DeviceListingTransactionHandle cci_start_device_listing_transaction() {
    return new DeviceListingTransaction;
}

void cci_fetch_device_listing(ChannelHandle channelHandle, DeviceListingTransactionHandle transactionHandle) {
    auto *channel = reinterpret_cast<Channel*>(channelHandle);
    auto *transaction = reinterpret_cast<DeviceListingTransaction*>(transactionHandle);

    transaction->responseMsg = channel->get_device_list();
}

size_t cci_get_device_listing(DeviceListingTransactionHandle transactionHandle, DeviceDescription **deviceDescriptionsOut) {
    auto *transaction = reinterpret_cast<DeviceListingTransaction*>(transactionHandle);
    size_t count = transaction->responseMsg.devices_size();

    *deviceDescriptionsOut = new DeviceDescription[count];
    transaction->deviceDescription = *deviceDescriptionsOut;

    for (int i = 0; i < count; i++){
        (*deviceDescriptionsOut)[i] = WrapDeviceDescriptionMessage(transaction->responseMsg.devices(i));
    }
    return count;
}

void cci_end_device_listing_transaction(DeviceListingTransactionHandle transactionHandle) {
    auto *transaction = reinterpret_cast<DeviceListingTransaction*>(transactionHandle);
    delete[] transaction->deviceDescription;
    delete transaction;
}

ClientListingTransactionHandle cci_start_client_listing_transaction() {
    return new ClientListingTransaction;
}

void cci_fetch_client_listing(ChannelHandle channelHandle, ClientListingTransactionHandle transactionHandle) {
    auto *channel = reinterpret_cast<Channel*>(channelHandle);
    auto *transaction = reinterpret_cast<ClientListingTransaction*>(transactionHandle);

    transaction->responseMsg = channel->get_client_list();
}

size_t cci_get_client_listing(ClientListingTransactionHandle transactionHandle, ClientDescription **clientDescriptionsOut) {
    auto *transaction = reinterpret_cast<ClientListingTransaction*>(transactionHandle);
    size_t count = transaction->responseMsg.clients_size();

    *clientDescriptionsOut = new ClientDescription[count];
    transaction->clientDescription = *clientDescriptionsOut;

    for (int i = 0; i < count; i++){
        (*clientDescriptionsOut)[i] = WrapClientDescriptionMessage(transaction->responseMsg.clients(i));
    }
    return count;
}

void cci_end_client_listing_transaction(ClientListingTransactionHandle transactionHandle) {
    auto *transaction = reinterpret_cast<ClientListingTransaction*>(transactionHandle);
    delete[] transaction->clientDescription;
    delete transaction;
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

DeviceConnectivityStatus get_device_connectivity_status_update() {
    if (hapticsChannel == nullptr)
        return (DeviceConnectivityStatus) {.isRight = false, .isConnected = false};

    DeviceConnectivityStatusMessage msg = hapticsChannel->check_if_device_connected();

    return (DeviceConnectivityStatus) {.isRight = msg.isright(), .isConnected = msg.isconnected()};
}

void start_haptic_transaction(intptr_t maestroPtr) {
    if (hapticsChannel != nullptr)
        hapticsChannel->send_start_haptic_transaction_message(maestroPtr == get_right_glove_pointer());
}

void end_haptic_transaction(intptr_t maestroPtr) {
    if (hapticsChannel != nullptr)
        hapticsChannel->send_end_haptic_transaction_message(maestroPtr == get_right_glove_pointer());
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
    return false;
}

void install_log_callback(log_callback callback) {
    if (hapticsChannel != nullptr)
        hapticsChannel->install_log_callback(callback);
}

#pragma endregion