//
// Created by john_contactci on 2/21/2022.
//

#pragma once

#include <vector>
#include <cstdint>

#include "common.pb.h"
#include "data.pb.h"
#include "haptics.pb.h"

namespace contactci::io {

    typedef void (*on_connect_callback)();
    typedef void (*on_disconnect_callback)();
    typedef void (*on_packet_received)();

    class Channel {
    public:
        Channel();

        void send_delimited(OpCode opcode, google::protobuf::Message& msg);
        std::string receive_delimited();

        // haptics.proto messages
        void send_set_dimension_message(uint32_t dimension, uint32_t flags, uint32_t bitmask, std::string values);
        void send_dimension_resume_message(uint32_t dimension, uint32_t flags, uint32_t bitmask);
        void send_dimension_suspend_message(uint32_t dimension, uint32_t flags, uint32_t bitmask);
        void send_dimension_status_message(uint32_t dimension, uint32_t flags, std::string values);
        void send_mdht_open_message(uint32_t id);
        void send_mdht_close_message(uint32_t id);
        void send_mdht_play_message(uint32_t id);
        void send_mdht_erase_message(uint32_t id);
        void send_mdht_suspend_message(uint32_t id);
        void send_mdht_resume_message(uint32_t id);
        void send_mdht_stop_message(uint32_t id);

        // data.proto messages
        void send_device_config_message(uint32_t chipID, std::string data);
        void send_firmware_update_request_message(uint32_t chipID);
        void send_firmware_update_negotiate_message(uint32_t chipID, bool ready, uint32_t packetSize);
        void send_firmware_update_chunk_message(uint32_t chipID, std::string data);
        void send_firmware_update_response_message(uint32_t chipID, FirmwareUpdateResponseMessage_fwResponse status);

        void register_on_connect_callback(on_connect_callback callback);
        void register_on_disconnect_callback(on_disconnect_callback callback);
        void register_on_packet_received_callback(on_packet_received callback);

        bool unregister_on_connect_callback(on_connect_callback callback);
        bool unregister_on_disconnect_callback(on_disconnect_callback callback);
        bool unregister_on_packet_received_callback(on_packet_received callback);

        void clear_on_connect_callbacks();
        void clear_on_disconnect_callbacks();
        void clear_on_packet_received();

    protected:
        void send(google::protobuf::Message& msg);

        void send_header(OpCode opcode, uint32_t length);
        void receive_header(OpCode &readOpcode, uint32_t& readLength);

        virtual void send(std::string data) = 0;
        virtual std::string receive(uint32_t numBytes) = 0;

        uint32_t HeaderSize;

    private:
        std::vector<on_connect_callback> on_connect_callbacks {};
        std::vector<on_disconnect_callback> on_disconnect_callbacks {};
        std::vector<on_packet_received> on_packet_received_callbacks {};
    };

    Channel::Channel() {
        PacketHeader temp;
        temp.set_opcode(1);
        temp.set_length(1);
        HeaderSize = temp.ByteSizeLong();
    }

    void Channel::send_delimited(OpCode opcode, google::protobuf::Message& msg) {
        uint32_t length = msg.ByteSizeLong();
        send_header(opcode, length);
        send( msg);
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

    void Channel::send_dimension_status_message(uint32_t dimension, uint32_t flags, std::string values) {
        DimensionStatusMessage toSend;
        toSend.set_dimension(dimension);
        toSend.set_flags(flags);
        toSend.set_values(values);
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

    void Channel::send_device_config_message(uint32_t chipID, std::string data) {
        DeviceConfigMessage toSend;
        toSend.set_chipid(chipID);
        toSend.set_data(data);
        send_delimited(OpCode::opDeviceConfigMessage, toSend);
    }

    void Channel::send_firmware_update_request_message(uint32_t chipID) {
        FirmwareUpdateRequestMessage toSend;
        toSend.set_chipid(chipID);
        send_delimited(OpCode::opFirmwareUpdateRequestMessage, toSend);
    }

    void Channel::send_firmware_update_negotiate_message(uint32_t chipID, bool ready, uint32_t packetSize) {
        FirmwareUpdateNegotiateMessage toSend;
        toSend.set_chipid(chipID);
        toSend.set_ready(ready);
        toSend.set_packetsize(packetSize);
        send_delimited(OpCode::opFirmwareUpdateNegotiateMessage, toSend);
    }

    void Channel::send_firmware_update_chunk_message(uint32_t chipID, std::string data) {
        FirmwareUpdateChunkMessage toSend;
        toSend.set_chipid(chipID);
        toSend.set_data(data);
        send_delimited(OpCode::opFirmwareUpdateChunkMessage, toSend);
    }

    void Channel::send_firmware_update_response_message(uint32_t chipID, FirmwareUpdateResponseMessage_fwResponse status) {
        FirmwareUpdateResponseMessage toSend;
        toSend.set_chipid(chipID);
        toSend.set_status(status);
        send_delimited(OpCode::opFirmwareUpdateResponseMessage, toSend);
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
}
