//
// Created by john_contactci on 3/20/2024.
//

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>

#include "lib_defs.h"
#include "haptic_state.h"

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

    class CCI_API_CLASS(HapticStateManager) {
    public:
        HapticStateManager(const std::string& memoryName, const std::string& eventName);
        ~HapticStateManager();

        const HapticState &get_left_haptic_state() const;
        const HapticState &get_right_haptic_state() const;

        bool wait_for_state_change(int timeout_ms) const;

    protected:
        class Implementation;
        HapticStateManager(std::unique_ptr<Implementation> &&implementation);
        std::unique_ptr<Implementation> implementation;
    };

    class CCI_API_CLASS(MutableHapticStateManager) : public HapticStateManager {
    public:
        MutableHapticStateManager(const std::string& memoryName, const std::string& eventName);

        HapticState& get_left_haptic_state();
        HapticState& get_right_haptic_state();

        void signal_haptic_state_changed();
    };

    class CCI_API_CLASS(Session) {
    public:
        Session();
        virtual ~Session();

        bool is_connected();
        std::vector<contactci::ClientDescription> get_client_list();
        std::vector<contactci::DeviceDescription> get_device_list();

        const std::optional<contactci::DeviceDescription> &get_left_device() const;
        const std::optional<contactci::DeviceDescription> &get_right_device() const;

        const std::string &get_service_version() const;
        bool is_service_interactive() const;

    protected:
        class Implementation;
        Session(std::unique_ptr<Session::Implementation> &&implementation);
        std::unique_ptr<Session::Implementation> implementation;
    };

    class CCI_API_CLASS(HapticSession) : public Session {
    public:
        HapticSession();
        virtual ~HapticSession();

        const HapticStateManager &get_global_haptic_state() const;

    protected:
        class Implementation;
        HapticSession(std::unique_ptr<HapticSession::Implementation> &&implementation);
        std::unique_ptr<HapticSession::Implementation> implementation;
    };

    class CCI_API_CLASS(MutableHapticSession) : public HapticSession {
    public:
        MutableHapticSession();
        ~MutableHapticSession();

        MutableHapticStateManager &get_session_haptic_state();

    private:
        class Implementation;
        std::unique_ptr<MutableHapticSession::Implementation> implementation;
    };
}
