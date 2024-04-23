//
// Created by john_contactci on 2/21/2022.
//

#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <iostream>

#include "common.pb.h"
#include "data.pb.h"
#include "haptics.pb.h"
#include "info.pb.h"

#include "spinlock.h"

namespace contactci {
    class Channel {
    public:
        Channel();

        void send_delimited(OpCode opcode, google::protobuf::Message& msg);
        std::string receive_delimited();

        DeviceListResponseMessage get_device_list();
        ClientListResponseMessage get_client_list();
        SessionInitializationResponseMessage initialize_session(bool isHapticSession, bool wantsHapticWriteAccess);

    protected:
        void send(google::protobuf::Message& msg);

        void send_header(OpCode opcode, uint32_t length);
        void receive_header(OpCode &readOpcode, uint32_t& readLength);

        virtual bool is_connected() = 0;
        virtual void send(std::string data) = 0;
        virtual std::string receive(uint32_t numBytes) = 0;
        virtual void flush() = 0;

        uint32_t HeaderSize;

    private:
        void send_device_list_request_message();
        void send_client_list_request_message();
        void send_initialize_session_request_message(bool isHapticSession, bool wantsHapticWriteAccess);

        SpinLock spinLock;
    };
}
