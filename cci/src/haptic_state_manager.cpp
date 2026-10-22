//
// Created by john_contactci on 3/18/2024.
//

#include "cci/haptic_state_manager.h"
#include "cci/shared_memory.h"
#include "cci/named_event.h"

#include <cstddef>

using namespace contactci;

// HapticState is shared byte-for-byte with the Service and the C# wrapper
// (SerializableHapticState) through shared memory, on every platform and CPU
// (x86_64 and arm64). Its layout is communication's contactci/haptic_state.h
// (DEV-16); any change to it is an ABI break, so it is pinned here against that
// header's constants, and test/unit/internal/haptic_state_test.cpp checks the same.
static_assert(sizeof(HapticState) == CONTACTCI_HAPTIC_STATE_SIZE, "HapticState layout changed");
static_assert(offsetof(HapticState, thumbForceFeedbackAmplitude) == CONTACTCI_HAPTIC_STATE_FORCE_FEEDBACK_AMPLITUDE_OFFSET, "HapticState layout changed");
static_assert(offsetof(HapticState, thumbForceFeedbackPosition) == CONTACTCI_HAPTIC_STATE_FORCE_FEEDBACK_POSITION_OFFSET, "HapticState layout changed");
static_assert(offsetof(HapticState, thumbVibrationAmplitude) == CONTACTCI_HAPTIC_STATE_VIBRATION_AMPLITUDE_OFFSET, "HapticState layout changed");
static_assert(offsetof(HapticState, thumbVibrationEffect) == CONTACTCI_HAPTIC_STATE_VIBRATION_EFFECT_OFFSET, "HapticState layout changed");
static_assert(offsetof(HapticState, thumbVibrationModifier) == CONTACTCI_HAPTIC_STATE_VIBRATION_MODIFIER_OFFSET, "HapticState layout changed");
static_assert(offsetof(HapticState, littleVibrationModifier) == CONTACTCI_HAPTIC_STATE_VIBRATION_MODIFIER_OFFSET + CONTACTCI_DIGIT_COUNT - 1, "HapticState layout changed");
static_assert(2 * sizeof(HapticState) == CONTACTCI_HAPTIC_REGION_SIZE, "a haptic state region is a left and a right HapticState");

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