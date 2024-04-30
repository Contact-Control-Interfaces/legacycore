//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <optional>
#include <memory>

#include "pipe_channel.h"
#include "haptic_state_manager.h"
#include "client.h"
#include "device.h"

#include <ccic/lib_defs.h>

namespace contactci {
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
