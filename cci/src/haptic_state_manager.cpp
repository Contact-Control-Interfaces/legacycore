//
// Created by john_contactci on 3/18/2024.
//

#include "haptic_state_manager.h"
#include "shared_memory.h"
#include "named_event.h"

using namespace contactci;

class HapticStateManager::Implementation {
public:
    Implementation(const std::string& memoryName, const std::string& eventName, bool canWrite);
    ~Implementation() = default;

    HapticState& get_left_haptic_state();
    HapticState& get_right_haptic_state();

    void signal_haptic_state_changed();
    bool wait_for_state_change(int timeout_ms);

private:
    SharedMemoryManager shared_memory;
    NamedEvent event;
};

HapticStateManager::Implementation::Implementation(const std::string& memoryName, const std::string& eventName, bool canWrite)
        : shared_memory(memoryName, sizeof(HapticState) * 2, canWrite), event(eventName, canWrite) {}

HapticState& HapticStateManager::Implementation::get_left_haptic_state() {
    return *static_cast<HapticState*>(shared_memory.get_memory());
}

HapticState& HapticStateManager::Implementation::get_right_haptic_state() {
    return *(static_cast<HapticState*>(shared_memory.get_memory()) + 1);
}

void HapticStateManager::Implementation::signal_haptic_state_changed() {
    event.set();
    event.reset();
}

bool HapticStateManager::Implementation::wait_for_state_change(int timeout_ms) {
    return event.wait(timeout_ms);
}

HapticStateManager::HapticStateManager(const std::string &memoryName, const std::string &eventName)
    : implementation(std::make_unique<HapticStateManager::Implementation>(memoryName, eventName, false)) {}

HapticStateManager::HapticStateManager(std::unique_ptr<Implementation> &&implementation)
    : implementation(std::move(implementation)) {}

HapticStateManager::~HapticStateManager() = default;

const HapticState &HapticStateManager::get_left_haptic_state() const {
    return implementation->get_left_haptic_state();
}

const HapticState &HapticStateManager::get_right_haptic_state() const {
    return implementation->get_right_haptic_state();
}

bool HapticStateManager::wait_for_state_change(int timeout_ms) const {
    return implementation->wait_for_state_change(timeout_ms);
}

MutableHapticStateManager::MutableHapticStateManager(const std::string &memoryName, const std::string &eventName)
    : HapticStateManager::HapticStateManager(std::make_unique<HapticStateManager::Implementation>(memoryName, eventName, true)) {}

HapticState &MutableHapticStateManager::get_left_haptic_state() {
    return const_cast<HapticState&>(HapticStateManager::get_left_haptic_state());
}

HapticState &MutableHapticStateManager::get_right_haptic_state() {
    return const_cast<HapticState&>(HapticStateManager::get_right_haptic_state());
}

void MutableHapticStateManager::signal_haptic_state_changed() {
    implementation->signal_haptic_state_changed();
}