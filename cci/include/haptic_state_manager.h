//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <cci_types.h>

#include "shared_memory.h"
#include "named_event.h"

#include <cci_lib_defs.h>

namespace contactci {
    class CCI_API_CLASS(HapticStateManager) {
    public:
        HapticStateManager(const std::string& memoryName, const std::string& eventName, bool canWrite);

        HapticState& get_left_haptic_state();
        HapticState& get_right_haptic_state();

        void signal_haptic_state_changed();
        bool wait_for_state_change(int timeout_ms);

    private:
        SharedMemoryManager shared_memory;
        NamedEvent event;
    };
}