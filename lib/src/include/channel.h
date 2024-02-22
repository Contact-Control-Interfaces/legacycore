//
// Created by john_contactci on 2/21/2022.
//

#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <mutex>
#include <iostream>

#include "common.pb.h"
#include "data.pb.h"
#include "haptics.pb.h"
#include "info.pb.h"

namespace contactci::io {

    typedef void (*on_connect_callback)();
    typedef void (*on_disconnect_callback)();
    typedef void (*on_packet_received)();

    typedef void(*log_callback)(const char*, size_t length);

    class Channel {
    public:
        Channel();

        void send_delimited(OpCode opcode, google::protobuf::Message& msg);
        std::string receive_delimited();

        DeviceConnectivityStatusMessage check_if_device_connected();

        // haptics.proto messages
        void send_start_haptic_transaction_message(bool isRight);
        void send_end_haptic_transaction_message(bool isRight);
        void send_vibration_effect_update_message(bool isRight, uint32_t effectCode, uint32_t modifiers, uint32_t bitmask);
        void send_vibration_amplitude_update_message(bool isRight, float amplitude, uint32_t bitmask);
        void send_force_feedback_update_message(bool isRight, float amplitude, uint32_t bitmask);
        void send_set_dimension_message(uint32_t dimension, uint32_t flags, uint32_t bitmask, std::string values);
        void send_dimension_resume_message(uint32_t dimension, uint32_t flags, uint32_t bitmask);
        void send_dimension_suspend_message(uint32_t dimension, uint32_t flags, uint32_t bitmask);
        void send_dimension_status_message(uint32_t dimension, uint32_t flags, uint32_t bitmask);
        void send_mdht_open_message(uint32_t id);
        void send_mdht_close_message(uint32_t id);
        void send_mdht_play_message(uint32_t id);
        void send_mdht_erase_message(uint32_t id);
        void send_mdht_suspend_message(uint32_t id);
        void send_mdht_resume_message(uint32_t id);
        void send_mdht_stop_message(uint32_t id);

        // data.proto messages
        void send_device_config_message(std::string data);
        // TODO firmware update message send functions?
        //void send_firmware_update_message(uint32_t chipID, std::string data, uint64_t checksum);
        //void send_firmware_update_response_message(FirmwareUpdateResponseMessage_fwResponse status);

        //info.proto messages
        void send_device_list_request_message();
        void send_client_list_request_message();
        void send_service_info_request_message();
        DeviceListResponseMessage get_device_list();
        ClientListResponseMessage get_client_list();
        ServiceInfoResponseMessage get_service_info();

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

        // common.proto messages
        void send_header(OpCode opcode, uint32_t length);
        void receive_header(OpCode &readOpcode, uint32_t& readLength);

        virtual void send(std::string data) = 0;
        virtual std::string receive(uint32_t numBytes) = 0;
        virtual void flush() = 0;

        void send_device_connectivity_message(bool isRight);
        void send_device_processing_status_message();

        uint32_t HeaderSize;
        log_callback logger = nullptr;
        std::mutex mutex;

    private:
        std::vector<on_connect_callback> on_connect_callbacks {};
        std::vector<on_disconnect_callback> on_disconnect_callbacks {};
        std::vector<on_packet_received> on_packet_received_callbacks {};
    };
}
