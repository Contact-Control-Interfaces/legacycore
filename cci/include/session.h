//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <memory>

#include "pipe_channel.h"
#include "haptic_state_manager.h"

#include <cci_lib_defs.h>

namespace contactci {
    class CCI_API_CLASS(ClientDescription) {
    public:
        ClientDescription(uint32_t process_id, std::string process_name);

        uint32_t get_process_id() const;
        const std::string &get_process_name() const;

    private:
        uint32_t process_id;
        std::string process_name;
    };

    class CCI_API_CLASS(DeviceDescription) {
    public:
        DeviceDescription(std::string product_line, std::string serial_number, bool is_right, bool is_connected);

        const std::string &get_product_line() const;
        const std::string &get_serial_number() const;
        bool get_is_right() const;
        bool get_is_connected() const;

    private:
        std::string product_line;
        std::string serial_number;
        bool is_right;
        bool is_connected;
    };

    class CCI_API_CLASS(ServiceInfo) {
    public:
        ServiceInfo(std::string version, bool is_interactive);

        const std::string &get_version() const;
        bool get_is_interactive() const;

    private:
        std::string version;
        bool is_interactive;
    };

    class CCI_API_CLASS(Session) {
    public:
        Session() = default;
        ~Session() = default;

        std::vector<contactci::DeviceDescription> get_device_list();
        std::vector<contactci::ClientDescription> get_client_list();
        contactci::ServiceInfo get_service_info();

        void attach_haptic_state(bool observeOnly);
        void detach_haptic_state();

        std::weak_ptr<HapticStateManager> get_global_haptic_state();
        std::weak_ptr<HapticStateManager> get_session_haptic_state();

    private:
        PipeChannel channel;
        std::shared_ptr<HapticStateManager> global_haptic_state;
        std::shared_ptr<HapticStateManager> session_haptic_state;
    };
}
