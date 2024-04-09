//
// Created by john_contactci on 2/7/2024.
//

#include "channel.h"

using namespace contactci;

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

void Channel::send_haptic_memory_access_request_message(bool wantsRead, bool wantsWrite) {
    HapticMemoryAccessRequestMessage toSend;

    toSend.set_wantsread(wantsRead);
    toSend.set_wantswrite(wantsWrite);

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

HapticMemoryAccessResponseMessage Channel::get_haptic_memory_access(bool observeOnly) {
    send_haptic_memory_access_request_message(true, !observeOnly);

    HapticMemoryAccessResponseMessage response;
    response.ParseFromString(receive_delimited());

    return response;
}
