//
// Created by john_contactci on 2/7/2024.
//

#include "cci/channel.h"
#include "cci/error.h"

#include <contactci/haptic_state.h>

#include <mutex>

using namespace contactci;

// communication/docs/linux-core-service-contract.md §3, "Protocol version": the Service must reply with the version
// Core requested, and anything else is a refusal, with no downgrade. Every Windows Service predates the field and
// sends 0, which Windows reads as 1. No Linux Service does, so Linux refuses 0 like any other version.
static bool is_requested_protocol_version(uint32_t replied) {
#ifdef _WIN32
    if (replied == 0)
        replied = 1;
#endif

    return replied == CONTACTCI_PROTOCOL_VERSION;
}

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

void Channel::send_initialize_session_request_message(bool isHapticSession, bool wantsHapticWriteAccess) {
    SessionInitializationRequestMessage toSend;
    toSend.set_ishapticsession(isHapticSession);
    toSend.set_wantshapticwriteaccess(wantsHapticWriteAccess);
    toSend.set_protocolversion(CONTACTCI_PROTOCOL_VERSION);

    send_delimited(OpCode::opSessionInitializationRequestMessage, toSend);
}

// Each request holds the lock until its response is read, so the device and
// client monitors sharing this channel never interleave. A lock_guard, so a
// failed send or receive releases it: otherwise every later request on this
// channel would spin forever.
DeviceListResponseMessage Channel::get_device_list() {
    std::lock_guard<SpinLock> guard(spinLock);
    send_device_list_request_message();
    DeviceListResponseMessage response;
    response.ParseFromString(receive_delimited());

    return response;
}

ClientListResponseMessage Channel::get_client_list() {
    std::lock_guard<SpinLock> guard(spinLock);
    send_client_list_request_message();
    ClientListResponseMessage response;
    response.ParseFromString(receive_delimited());

    return response;
}

SessionInitializationResponseMessage Channel::initialize_session(bool isHapticSession, bool wantsHapticWriteAccess) {
    std::lock_guard<SpinLock> guard(spinLock);
    send_initialize_session_request_message(isHapticSession, wantsHapticWriteAccess);

    OpCode opcode;
    uint32_t length;
    receive_header(opcode, length);

    // A reply that isn't opcode 45, or doesn't parse, is a broken Service rather than a refusal. Checked before the
    // body is read, so a wrong header's length is never trusted
    if (opcode != OpCode::opSessionInitializationResponseMessage)
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_READ);

    SessionInitializationResponseMessage response;

    if (!response.ParseFromString(receive(length)))
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_READ);

    // Before anything is opened: a Service on another version may lay out or signal the objects differently
    if (!is_requested_protocol_version(response.protocolversion()))
        throw contactci::Exception(CCI_ERR_SESSION_PROTOCOL_MISMATCH);

    return response;
}
