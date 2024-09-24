//
// Created by john_contactci on 2/7/2024.
//

#include "cci/channel.h"

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

void Channel::send_initialize_session_request_message(bool isHapticSession, bool wantsHapticWriteAccess) {
    SessionInitializationRequestMessage toSend;
    toSend.set_ishapticsession(isHapticSession);
    toSend.set_wantshapticwriteaccess(wantsHapticWriteAccess);

    send_delimited(OpCode::opSessionInitializationRequestMessage, toSend);
}


void Channel::send_wav_table_request_message(bool terse) {
    WavTableListMessage toSend;
    toSend.set_terse(terse);

    send_delimited(OpCode::opWavTableListMessage, toSend);
}

void Channel::send_wav_data_message(uint32_t sampleRate, const std::vector<const float> &samples, std::optional<std::string> description) {
    UploadClientWaveformMessage toSend;
    toSend.set_frequency(sampleRate);
    if(description.has_value())
        toSend.set_descriptor(description.value());
    for(const auto f : samples)
        toSend.add_samples(f);

    send_delimited(OpCode::opWavDataMessage, toSend);
}

void Channel::send_wav_delete_message(uint32_t index) {
    WavDeleteMessage toSend;
    toSend.set_index(index);

    send_delimited(OpCode::opWavDeleteMessage, toSend);
}

DeviceListResponseMessage Channel::get_device_list() {
    spinLock.lock();
    send_device_list_request_message();
    DeviceListResponseMessage response;
    response.ParseFromString(receive_delimited());
    spinLock.unlock();

    return response;
}

ClientListResponseMessage Channel::get_client_list() {
    spinLock.lock();
    send_client_list_request_message();
    ClientListResponseMessage response;
    response.ParseFromString(receive_delimited());
    spinLock.unlock();

    return response;
}

SessionInitializationResponseMessage Channel::initialize_session(bool isHapticSession, bool wantsHapticWriteAccess) {
    spinLock.lock();
    send_initialize_session_request_message(isHapticSession, wantsHapticWriteAccess);
    SessionInitializationResponseMessage response;
    response.ParseFromString(receive_delimited());
    spinLock.unlock();

    return response;
}

WavTableListMessage Channel::get_wav_table(bool terse) {
    spinLock.lock();
    send_wav_table_request_message(terse);
    WavTableListMessage response;
    response.ParseFromString(receive_delimited());
    spinLock.unlock();

    return response;
}

UploadClientWaveformResponseMessage Channel::upload_waveform(uint32_t sampleRate,
                                std::vector<const float> samples, std::optional<std::string> descriptor) {
    spinLock.lock();
    send_wav_data_message(sampleRate, samples, descriptor);
    UploadClientWaveformResponseMessage response;
    response.ParseFromString(receive_delimited());
    spinLock.unlock();

    return response;
}

void Channel::delete_waveform(uint32_t index) {
    spinLock.lock();
    send_wav_delete_message(index);
    spinLock.unlock();
}
