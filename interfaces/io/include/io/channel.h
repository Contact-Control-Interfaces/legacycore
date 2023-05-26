//
// Created by john_contactci on 2/21/2022.
//

#pragma once

#include <vector>
#include <cstdint>

#include "common.pb.h"
#include "data.pb.h"
#include "haptics.pb.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! BEWARE THAT YOU DON'T RECURSE ON THIS MACRO !!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Doing so will deadlock the thread
// Default to thread safety (locking), but allow an override for single-threaded builds with performance concerns
#if CCI_THREAD_UNSAFE
#define IO_LOCKED(a) a
#else
#define IO_LOCKED(a) mutex.lock();\
                     a\
                     mutex.unlock();
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! BEWARE THAT YOU DON'T RECURSE ON THIS MACRO !!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

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

        bool check_if_device_connected(bool isRight);
        bool check_if_device_processing();

        void install_log_callback(log_callback callback);
        void log(const std::string& text);

        // haptics.proto messages
        void send_start_haptic_transaction_message(bool isRight);
        void send_end_haptic_transaction_message(bool isRight);
        void send_vibration_update_message(bool isRight, uint32_t effectCode, uint32_t modifiers, uint32_t bitmask);
        void send_force_feedback_update_message(bool isRight, float amplitude, uint32_t bitmask);
        void send_force_feedback_pid_update_message(bool isRight, bool isAbsolute, float target, uint32_t bitmask); // Not implemented yet
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
        void send_device_config_message(std::string data);
        // TODO firmware update message send functions?
        //void send_firmware_update_message(uint32_t chipID, std::string data, uint64_t checksum);
        //void send_firmware_update_response_message(FirmwareUpdateResponseMessage_fwResponse status);

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

    Channel::Channel() {
        PacketHeader temp;
        temp.set_opcode(1);
        temp.set_length(1);
        HeaderSize = temp.ByteSizeLong();
    }

    void Channel::install_log_callback(log_callback callback) {
        logger = callback;
    }

    void Channel::log(const std::string& text) {
        // Output to stdout
        std::cout << text << std::endl;

        // Also output to log if callback installed
        if (logger != nullptr) {
            logger(text.c_str(), text.length());
        }
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

    bool Channel::check_if_device_connected(bool isRight) {
        DeviceConnectivityStatusReply reply;

        IO_LOCKED(
            this->send_device_connectivity_message(isRight);
            reply.ParseFromString(receive_delimited());
        )

        return reply.isconnected();
    }

    bool Channel::check_if_device_processing() {
        DeviceProcessingStatusReply reply;

        IO_LOCKED(
            this->send_device_processing_status_message();
            reply.ParseFromString(receive_delimited());
        )

        return reply.isprocessing();
    }

    void Channel::send_start_haptic_transaction_message(bool isRight) {
        IO_LOCKED(
            StartHapticTransactionMessage toSend;
            toSend.set_isright(isRight);
            send_delimited(OpCode::opStartHapticTransactionMessage, toSend);
        )
    }

    void Channel::send_end_haptic_transaction_message(bool isRight) {
        IO_LOCKED(
            EndHapticTransactionMessage toSend;
            toSend.set_isright(isRight);
            send_delimited(OpCode::opEndHapticTransactionMessage, toSend);
        )
    }

    void Channel::send_vibration_update_message(bool isRight, uint32_t effectCode, uint32_t modifiers, uint32_t bitmask) {
        IO_LOCKED(
            VibrationUpdateMessage toSend;
            toSend.set_isright(isRight);
            toSend.set_effectcode(effectCode);
            toSend.set_modifiers(modifiers);
            toSend.set_bitmask(bitmask);
            send_delimited(OpCode::opVibrationUpdateMessage, toSend);
        )
    }

    void Channel::send_force_feedback_update_message(bool isRight, float amplitude, uint32_t bitmask) {
        IO_LOCKED(
            ForceFeedbackUpdateMessage toSend;
            toSend.set_isright(isRight);
            toSend.set_amplitude(amplitude);
            toSend.set_bitmask(bitmask);
            send_delimited(OpCode::opForceFeedbackUpdateMessage, toSend);
        )
    }

    void Channel::send_force_feedback_pid_update_message(bool isRight, bool isAbsolute, float target, uint32_t bitmask) {
        IO_LOCKED(
            ForceFeedbackPIDUpdateMessage toSend;
	    toSend.set_isright(isRight);
	    toSend.set_isabsolute(isAbsolute);
	    toSend.set_target(target);
	    toSend.set_bitmask(bitmask);
	    send_delimited(OpCode::opForceFeedbackPIDUpdateMessage, toSend
        )
    }

    void Channel::send_device_connectivity_message(bool isRight) {
        DeviceConnectivityStatusMessage toSend;
        toSend.set_isright(isRight);
        send_delimited(OpCode::opDeviceConnectivityStatusMessage, toSend);
    }

    void Channel::send_device_processing_status_message() {
        DeviceProcessingStatusMessage toSend;
        send_delimited(OpCode::opDeviceProcessingStatusMessage, toSend);
    }

    void Channel::send_set_dimension_message(uint32_t dimension, uint32_t flags, uint32_t bitmask, std::string values) {
        IO_LOCKED(
            SetDimensionMessage toSend;
            toSend.set_dimension(dimension);
            toSend.set_flags(flags);
            toSend.set_bitmask(bitmask);
            toSend.set_value(values);
            send_delimited(OpCode::opSetDimensionMessage, toSend);
        )
    }

    void Channel::send_dimension_suspend_message(uint32_t dimension, uint32_t flags, uint32_t bitmask) {
        IO_LOCKED(
            DimensionSuspendMessage toSend;
            toSend.set_dimension(dimension);
            toSend.set_flags(flags);
            toSend.set_bitmask(bitmask);
            send_delimited(OpCode::opDimensionSuspendMessage, toSend);
        )
    }

    void Channel::send_dimension_resume_message(uint32_t dimension, uint32_t flags, uint32_t bitmask) {
        IO_LOCKED(
            DimensionResumeMessage toSend;
            toSend.set_dimension(dimension);
            toSend.set_flags(flags);
            toSend.set_bitmask(bitmask);
            send_delimited(OpCode::opDimensionResumeMessage, toSend);
        )
    }

    void Channel::send_dimension_status_message(uint32_t dimension, uint32_t flags, std::string values) {
        IO_LOCKED(
            DimensionStatusMessage toSend;
            toSend.set_dimension(dimension);
            toSend.set_flags(flags);
            toSend.set_values(values);
            send_delimited(OpCode::opDimensionStatusMessage, toSend);
        )
    }

    void Channel::send_mdht_open_message(uint32_t id) {
        IO_LOCKED(
            MDHTOpenMessage toSend;
            toSend.set_id(id);
            send_delimited(OpCode::opMDHTOpenMessage, toSend);
        )
    }

    void Channel::send_mdht_close_message(uint32_t id) {
        IO_LOCKED(
            MDHTCloseMessage toSend;
            toSend.set_id(id);
            send_delimited(OpCode::opMDHTCloseMessage, toSend);
        )
    }

    void Channel::send_mdht_play_message(uint32_t id) {
        IO_LOCKED(
            MDHTPlayMessage toSend;
            toSend.set_id(id);
            send_delimited(OpCode::opMDHTPlayMessage, toSend);
        )
    }

    void Channel::send_mdht_erase_message(uint32_t id) {
        IO_LOCKED(
            MDHTEraseMessage toSend;
            toSend.set_id(id);
            send_delimited(OpCode::opMDHTEraseMessage, toSend);
        )
    }

    void Channel::send_mdht_suspend_message(uint32_t id) {
        IO_LOCKED(
            MDHTSuspendMessage toSend;
            toSend.set_id(id);
            send_delimited(OpCode::opMDHTSuspendMessage, toSend);
        )
    }

    void Channel::send_mdht_resume_message(uint32_t id) {
        IO_LOCKED(
            MDHTResumeMessage toSend;
            toSend.set_id(id);
            send_delimited(OpCode::opMDHTResumeMessage, toSend);
        )
    }

    void Channel::send_mdht_stop_message(uint32_t id) {
        IO_LOCKED(
            MDHTStopMessage toSend;
            toSend.set_id(id);
            send_delimited(OpCode::opMDHTStopMessage, toSend);
        )
    }

    void Channel::send_device_config_message(std::string data) {
        IO_LOCKED(
            DeviceConfigMessage toSend;
            toSend.set_data(data);
            send_delimited(OpCode::opDeviceConfigMessage, toSend);
        )
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
