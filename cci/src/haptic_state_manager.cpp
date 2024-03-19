//
// Created by john_contactci on 3/18/2024.
//

#include "haptic_state_manager.h"

using namespace contactci;

HapticStateManager::HapticStateManager(const std::string& memoryName, const std::string& eventName, bool canWrite)
        : shared_memory(memoryName, sizeof(HapticState) * 2, canWrite), event(eventName, canWrite) { }

HapticState& HapticStateManager::get_left_haptic_state() {
    return *static_cast<HapticState*>(shared_memory.get_memory());
}

HapticState& HapticStateManager::get_right_haptic_state() {
    return *(static_cast<HapticState*>(shared_memory.get_memory()) + 1);
}

void HapticStateManager::signal_haptic_state_changed() {
    event.set();
    event.reset();
}

bool HapticStateManager::wait_for_state_change(int timeout_ms) {
    return event.wait(timeout_ms);
}