//
// Created by john_contactci on 3/9/2022.
//

#include "core.h"
#include "haptics.h"
#include "info.h"
#include "pipe_channel.h"
#include "shared_memory.h"

using namespace contactci::io;

contactci::io::SharedMemoryManager *leftSharedMemoryManager = nullptr;
contactci::io::SharedMemoryManager *rightSharedMemoryManager = nullptr;

contactci::io::HapticState *hapticState[2];

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

typedef struct {
    ServiceInfoResponseMessage responseMsg;
} ServiceInfoTransaction;

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

void cci_channel_send_set_dimension_message(ChannelHandle channel, unsigned int dimension, unsigned int flags, unsigned int bitmask, const char* values, int valuesCount){
    auto *channel_ptr = reinterpret_cast<Channel*>(channel);
    channel_ptr->send_set_dimension_message(dimension, flags, bitmask, std::string(values, valuesCount));
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

ServiceInfoTransactionHandle cci_start_service_info_transaction() {
    return new ServiceInfoTransaction;
}

void cci_fetch_service_info(ChannelHandle channelHandle, ServiceInfoTransactionHandle transactionHandle) {
    auto *channel = reinterpret_cast<Channel*>(channelHandle);
    auto *transaction = reinterpret_cast<ServiceInfoTransaction*>(transactionHandle);

    transaction->responseMsg = channel->get_service_info();
}

void cci_get_service_info(ServiceInfoTransactionHandle transactionHandle, ServiceInfo *serviceInfoOut) {
    auto *transaction = reinterpret_cast<ServiceInfoTransaction*>(transactionHandle);

    serviceInfoOut->version = transaction->responseMsg.version().c_str();
    serviceInfoOut->isInteractive = transaction->responseMsg.isinteractive();
}

void cci_end_service_info_transaction(ServiceInfoTransactionHandle transactionHandle) {
    auto *transaction = reinterpret_cast<ServiceInfoTransaction*>(transactionHandle);
    delete transaction;
}

#pragma region Old C API

bool start_maestro_detection_service() {
    try {
        if (leftSharedMemoryManager == nullptr) {
            leftSharedMemoryManager = new SharedMemoryManager(false);
            hapticState[get_left_glove_pointer()] = leftSharedMemoryManager->getHapticStateMapping();
        }

        if (rightSharedMemoryManager == nullptr) {
            rightSharedMemoryManager = new SharedMemoryManager(true);
            hapticState[get_right_glove_pointer()] = rightSharedMemoryManager->getHapticStateMapping();
        }

        return true;
    } catch (std::exception &e) {
        // TODO output error?
        return false;
    }
}

bool stop_maestro_detection_service() {
    delete leftSharedMemoryManager;
    leftSharedMemoryManager = nullptr;
    hapticState[get_left_glove_pointer()] = nullptr;

    delete rightSharedMemoryManager;
    rightSharedMemoryManager = nullptr;
    hapticState[get_right_glove_pointer()] = nullptr;

    return true;
}

inline intptr_t const get_left_glove_pointer() {
    return 0; // this isn't a pointer
}

inline intptr_t const get_right_glove_pointer() {
    return 1; // this isn't a pointer
}

void start_haptic_transaction(intptr_t maestroPtr) {
    // Intentionally empty
}

void end_haptic_transaction(intptr_t maestroPtr) {
    if (maestroPtr == get_right_glove_pointer())
        rightSharedMemoryManager->signalEvent();
    else
        leftSharedMemoryManager->signalEvent();
}

void set_thumb_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier) {
    hapticState[maestroPtr]->thumbVibrationEffect = effect;
    hapticState[maestroPtr]->thumbVibrationModifier = modifier;
    hapticState[maestroPtr]->thumbVibrationAmplitude = 0;
}

void set_index_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier) {
    hapticState[maestroPtr]->indexVibrationEffect = effect;
    hapticState[maestroPtr]->indexVibrationModifier = modifier;
    hapticState[maestroPtr]->indexVibrationAmplitude = 0;
}

void set_middle_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier) {
    hapticState[maestroPtr]->middleVibrationEffect = effect;
    hapticState[maestroPtr]->middleVibrationModifier = modifier;
    hapticState[maestroPtr]->middleVibrationAmplitude = 0;
}

void set_ring_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier) {
    hapticState[maestroPtr]->ringVibrationEffect = effect;
    hapticState[maestroPtr]->ringVibrationModifier = modifier;
    hapticState[maestroPtr]->ringVibrationAmplitude = 0;
}

void set_little_vibration_effect(intptr_t maestroPtr, uint8_t effect, uint8_t modifier) {
    hapticState[maestroPtr]->littleVibrationEffect = effect;
    hapticState[maestroPtr]->littleVibrationModifier = modifier;
    hapticState[maestroPtr]->littleVibrationAmplitude = 0;
}

void set_thumb_vibration_amplitude(intptr_t maestroPtr, float amplitude) {
    hapticState[maestroPtr]->thumbVibrationAmplitude = amplitude;
    hapticState[maestroPtr]->thumbVibrationEffect = 0;
    hapticState[maestroPtr]->thumbVibrationModifier = 0;
}

void set_index_vibration_amplitude(intptr_t maestroPtr, float amplitude) {
    hapticState[maestroPtr]->indexVibrationAmplitude = amplitude;
    hapticState[maestroPtr]->indexVibrationEffect = 0;
    hapticState[maestroPtr]->indexVibrationModifier = 0;
}

void set_middle_vibration_amplitude(intptr_t maestroPtr, float amplitude) {
    hapticState[maestroPtr]->middleVibrationAmplitude = amplitude;
    hapticState[maestroPtr]->middleVibrationEffect = 0;
    hapticState[maestroPtr]->middleVibrationModifier = 0;
}

void set_ring_vibration_amplitude(intptr_t maestroPtr, float amplitude) {
    hapticState[maestroPtr]->ringVibrationAmplitude = amplitude;
    hapticState[maestroPtr]->ringVibrationEffect = 0;
    hapticState[maestroPtr]->ringVibrationModifier = 0;
}

void set_little_vibration_amplitude(intptr_t maestroPtr, float amplitude) {
    hapticState[maestroPtr]->littleVibrationAmplitude = amplitude;
    hapticState[maestroPtr]->littleVibrationEffect = 0;
    hapticState[maestroPtr]->littleVibrationModifier = 0;
}

void set_thumb_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude) {
    hapticState[maestroPtr]->thumbMotorAmplitude = amplitude;
}

void set_index_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude) {
    hapticState[maestroPtr]->indexMotorAmplitude = amplitude;
}

void set_middle_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude) {
    hapticState[maestroPtr]->middleMotorAmplitude = amplitude;
}

void set_ring_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude) {
    hapticState[maestroPtr]->ringMotorAmplitude = amplitude;
}

void set_little_motor_amplitude(intptr_t maestroPtr, uint8_t amplitude) {
    hapticState[maestroPtr]->littleMotorAmplitude = amplitude;
}

#pragma endregion