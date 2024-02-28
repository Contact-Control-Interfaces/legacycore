//
// Created by john_contactci on 2/7/2024.
//

#include "channel.h"

using namespace contactci::io;

Channel::Channel() {
    PacketHeader temp;
    temp.set_opcode(1);
    temp.set_length(1);
    HeaderSize = temp.ByteSizeLong();
}

void Channel::send_delimited(OpCode opcode, google::protobuf::Message& msg) {
    uint32_t length = msg.ByteSizeLong();
    send_header(opcode, length);
    send(msg);
}

std::string Channel::receive_delimited() {
    OpCode opcode;
    uint32_t length;
    receive_header(opcode, length);
    return receive(length);
}

void Channel::send(google::protobuf::Message& msg) {
    std::string message;
    msg.SerializeToString(&message);
    send(message);
}

void Channel::send_header(OpCode opcode, uint32_t length) {
    PacketHeader header;
    header.set_opcode(opcode);
    header.set_length(length);
    send(header);
}

void Channel::receive_header(OpCode &readOpcode, uint32_t& readLength) {
    PacketHeader header;
    header.ParseFromString(receive(HeaderSize));
    readOpcode = (OpCode) header.opcode();
    readLength = header.length();
}

DeviceConnectivityStatusMessage Channel::check_if_device_connected() {
    DeviceConnectivityStatusMessage message;

    message.ParseFromString(receive_delimited());

    return message;
}

void Channel::send_start_haptic_transaction_message(bool isRight) {
    StartHapticTransactionMessage toSend;
    toSend.set_isright(isRight);
    send_delimited(OpCode::opStartHapticTransactionMessage, toSend);
}

void Channel::send_end_haptic_transaction_message(bool isRight) {
    EndHapticTransactionMessage toSend;
    toSend.set_isright(isRight);
    send_delimited(OpCode::opEndHapticTransactionMessage, toSend);
}

void Channel::send_vibration_effect_update_message(bool isRight, uint32_t effectCode, uint32_t modifiers, uint32_t bitmask) {
    VibrationEffectUpdateMessage toSend;
    toSend.set_isright(isRight);
    toSend.set_effectcode(effectCode);
    toSend.set_modifiers(modifiers);
    toSend.set_bitmask(bitmask);
    send_delimited(OpCode::opVibrationEffectUpdateMessage, toSend);
}

void Channel::send_vibration_amplitude_update_message(bool isRight, float amplitude, uint32_t bitmask) {
    VibrationAmplitudeUpdateMessage toSend;
    toSend.set_isright(isRight);
    toSend.set_amplitude(amplitude);
    toSend.set_bitmask(bitmask);
    send_delimited(OpCode::opVibrationAmplitudeUpdateMessage, toSend);
}

void Channel::send_force_feedback_update_message(bool isRight, float amplitude, uint32_t bitmask) {
    ForceFeedbackUpdateMessage toSend;
    toSend.set_isright(isRight);
    toSend.set_amplitude(amplitude);
    toSend.set_bitmask(bitmask);
    send_delimited(OpCode::opForceFeedbackUpdateMessage, toSend);
}

void Channel::send_set_dimension_message(uint32_t dimension, uint32_t flags, uint32_t bitmask, std::string values) {
    SetDimensionMessage toSend;
    toSend.set_dimension(dimension);
    toSend.set_flags(flags);
    toSend.set_bitmask(bitmask);
    toSend.set_value(values);
    send_delimited(OpCode::opSetDimensionMessage, toSend);
}

void Channel::send_dimension_suspend_message(uint32_t dimension, uint32_t flags, uint32_t bitmask) {
    DimensionSuspendMessage toSend;
    toSend.set_dimension(dimension);
    toSend.set_flags(flags);
    toSend.set_bitmask(bitmask);
    send_delimited(OpCode::opDimensionSuspendMessage, toSend);
}

void Channel::send_dimension_resume_message(uint32_t dimension, uint32_t flags, uint32_t bitmask) {
    DimensionResumeMessage toSend;
    toSend.set_dimension(dimension);
    toSend.set_flags(flags);
    toSend.set_bitmask(bitmask);
    send_delimited(OpCode::opDimensionResumeMessage, toSend);
}

void Channel::send_dimension_status_message(uint32_t dimension, uint32_t flags, uint32_t bitmask) {
    DimensionStatusMessage toSend;
    toSend.set_dimension(dimension);
    toSend.set_flags(flags);
    toSend.set_bitmask(bitmask);
    send_delimited(OpCode::opDimensionStatusMessage, toSend);
}

void Channel::send_mdht_open_message(uint32_t id) {
    MDHTOpenMessage toSend;
    toSend.set_id(id);
    send_delimited(OpCode::opMDHTOpenMessage, toSend);
}

void Channel::send_mdht_close_message(uint32_t id) {
    MDHTCloseMessage toSend;
    toSend.set_id(id);
    send_delimited(OpCode::opMDHTCloseMessage, toSend);
}

void Channel::send_mdht_play_message(uint32_t id) {
    MDHTPlayMessage toSend;
    toSend.set_id(id);
    send_delimited(OpCode::opMDHTPlayMessage, toSend);
}

void Channel::send_mdht_erase_message(uint32_t id) {
    MDHTEraseMessage toSend;
    toSend.set_id(id);
    send_delimited(OpCode::opMDHTEraseMessage, toSend);
}

void Channel::send_mdht_suspend_message(uint32_t id) {
    MDHTSuspendMessage toSend;
    toSend.set_id(id);
    send_delimited(OpCode::opMDHTSuspendMessage, toSend);
}

void Channel::send_mdht_resume_message(uint32_t id) {
    MDHTResumeMessage toSend;
    toSend.set_id(id);
    send_delimited(OpCode::opMDHTResumeMessage, toSend);
}

void Channel::send_mdht_stop_message(uint32_t id) {
    MDHTStopMessage toSend;
    toSend.set_id(id);
    send_delimited(OpCode::opMDHTStopMessage, toSend);
}

void Channel::send_device_config_message(std::string data) {
    DeviceConfigMessage toSend;
    toSend.set_data(data);
    send_delimited(OpCode::opDeviceConfigMessage, toSend);
}

void Channel::send_device_list_request_message() {
    DeviceListRequestMessage toSend;
    send_delimited(OpCode::opDeviceListRequestMessage, toSend);
}

void Channel::send_client_list_request_message() {
    ClientListRequestMessage toSend;
    send_delimited(OpCode::opClientListRequestMessage, toSend);
}

void Channel::send_service_info_request_message() {
    ServiceInfoRequestMessage toSend;
    send_delimited(OpCode::opServiceInfoRequestMessage, toSend);
}

void Channel::send_haptic_memory_access_request_message() {
    HapticMemoryAccessRequestMessage toSend;

    toSend.set_wantsread(true);
    toSend.set_wantswrite(true);

    send_delimited(OpCode::opHapticMemoryAccessRequestMessage, toSend);
}

DeviceListResponseMessage Channel::get_device_list() {
    send_device_list_request_message();

    DeviceListResponseMessage response;
    response.ParseFromString(receive_delimited());
    return response;
}

ClientListResponseMessage Channel::get_client_list() {
    send_client_list_request_message();

    ClientListResponseMessage response;
    response.ParseFromString(receive_delimited());
    return response;
}

ServiceInfoResponseMessage Channel::get_service_info() {
    send_service_info_request_message();

    ServiceInfoResponseMessage response;
    response.ParseFromString(receive_delimited());
    return response;
}

HapticMemoryAccessResponseMessage Channel::get_haptic_memory_access() {
    send_haptic_memory_access_request_message();

    HapticMemoryAccessResponseMessage response;
    response.ParseFromString(receive_delimited());

    return response;
}

void Channel::register_on_connect_callback(on_connect_callback callback) {
    on_connect_callbacks.push_back(callback);
}

void Channel::register_on_disconnect_callback(on_disconnect_callback callback) {
    on_disconnect_callbacks.push_back(callback);
}

void Channel::register_on_packet_received_callback(on_packet_received callback) {
    on_packet_received_callbacks.push_back(callback);
}

bool Channel::unregister_on_connect_callback(on_connect_callback callback) {
    return std::erase(on_connect_callbacks, callback) > 0;
}

bool Channel::unregister_on_disconnect_callback(on_disconnect_callback callback) {
    return std::erase(on_disconnect_callbacks, callback) > 0;
}

bool Channel::unregister_on_packet_received_callback(on_packet_received callback) {
    return std::erase(on_packet_received_callbacks, callback) > 0;
}

void Channel::clear_on_connect_callbacks() {
    on_connect_callbacks.clear();
}

void Channel::clear_on_disconnect_callbacks() {
    on_disconnect_callbacks.clear();
}

void Channel::clear_on_packet_received() {
    on_packet_received_callbacks.clear();
}