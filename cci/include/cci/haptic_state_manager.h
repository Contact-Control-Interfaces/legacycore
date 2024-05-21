//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <ccic/haptic_state.h>
#include <ccic/lib_defs.h>

#include <string>
#include <memory>

namespace contactci {
    class CCI_API_CLASS(HapticStateManager) {
    public:
        HapticStateManager(const std::string &memoryName, const std::string &eventName);
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
        MutableHapticStateManager(const std::string &memoryName, const std::string &eventName);

        HapticState &get_left_haptic_state();
        HapticState &get_right_haptic_state();

        void signal_haptic_state_changed();
    };
}