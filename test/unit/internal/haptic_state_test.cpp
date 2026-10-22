#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <type_traits>

#include <gtest/gtest.h>

#include "cci/haptic_state_manager.h"
#include "test_objects.h"

using testsupport::TestEvent;
using testsupport::TestSharedMemory;

// HapticState is shared byte-for-byte with the Windows service and the C#
// wrapper (SerializableHapticState) through shared memory, and on Linux with
// the Service, whose RawHapticState is the same struct. Any change to this
// layout is an ABI break for all of them, so it is pinned here: against the
// contract's numbers, and against communication's contactci/haptic_state.h,
// which defines it (DEV-16, DEV-18).
TEST(HapticStateLayout, MatchesTheSharedMemoryContract) {
    EXPECT_EQ(sizeof(HapticState), 72u);
    EXPECT_EQ(offsetof(HapticState, thumbForceFeedbackAmplitude), 0u);
    EXPECT_EQ(offsetof(HapticState, thumbForceFeedbackPosition), 20u);
    EXPECT_EQ(offsetof(HapticState, thumbVibrationAmplitude), 40u);
    EXPECT_EQ(offsetof(HapticState, thumbVibrationEffect), 60u);
    EXPECT_EQ(offsetof(HapticState, thumbVibrationModifier), 65u);
    EXPECT_EQ(offsetof(HapticState, littleVibrationModifier), 69u);
}

TEST(HapticStateLayout, IsTheSharedHeadersStruct) {
    EXPECT_TRUE((std::is_same_v<HapticState, ContactCiHapticState>));
    EXPECT_EQ(sizeof(HapticState), std::size_t{CONTACTCI_HAPTIC_STATE_SIZE});
    EXPECT_EQ(offsetof(HapticState, thumbForceFeedbackAmplitude), std::size_t{CONTACTCI_HAPTIC_STATE_FORCE_FEEDBACK_AMPLITUDE_OFFSET});
    EXPECT_EQ(offsetof(HapticState, thumbForceFeedbackPosition), std::size_t{CONTACTCI_HAPTIC_STATE_FORCE_FEEDBACK_POSITION_OFFSET});
    EXPECT_EQ(offsetof(HapticState, thumbVibrationAmplitude), std::size_t{CONTACTCI_HAPTIC_STATE_VIBRATION_AMPLITUDE_OFFSET});
    EXPECT_EQ(offsetof(HapticState, thumbVibrationEffect), std::size_t{CONTACTCI_HAPTIC_STATE_VIBRATION_EFFECT_OFFSET});
    EXPECT_EQ(offsetof(HapticState, thumbVibrationModifier), std::size_t{CONTACTCI_HAPTIC_STATE_VIBRATION_MODIFIER_OFFSET});
    EXPECT_EQ(2 * sizeof(HapticState), std::size_t{CONTACTCI_HAPTIC_REGION_SIZE});
    EXPECT_EQ(std::size_t{CONTACTCI_HAPTIC_REGION_LEFT_OFFSET}, 0u);
    EXPECT_EQ(std::size_t{CONTACTCI_HAPTIC_REGION_RIGHT_OFFSET}, sizeof(HapticState));
    EXPECT_EQ(sizeof(ContactCiChangeCounter), std::size_t{CONTACTCI_CHANGE_COUNTER_SIZE});
    EXPECT_EQ(CONTACTCI_CHANGE_COUNTER_INITIAL, 0);
}

TEST(HapticStateManager, LeftAndRightAreConsecutiveStatesInSharedMemory) {
    TestSharedMemory memory;
    TestEvent event;
    memory.left()->thumbForceFeedbackAmplitude = 0.1f;
    memory.right()->thumbForceFeedbackAmplitude = 0.9f;

    contactci::HapticStateManager manager(memory.name(), event.name());

    EXPECT_FLOAT_EQ(manager.get_left_haptic_state().thumbForceFeedbackAmplitude, 0.1f);
    EXPECT_FLOAT_EQ(manager.get_right_haptic_state().thumbForceFeedbackAmplitude, 0.9f);
    EXPECT_EQ(&manager.get_right_haptic_state(), &manager.get_left_haptic_state() + 1);
}

TEST(HapticStateManager, ReadsAreLiveNotCopies) {
    TestSharedMemory memory;
    TestEvent event;
    contactci::HapticStateManager manager(memory.name(), event.name());

    memory.right()->middleVibrationAmplitude = 0.4f;

    EXPECT_FLOAT_EQ(manager.get_right_haptic_state().middleVibrationAmplitude, 0.4f);
}

TEST(HapticStateManager, WaitForStateChangeFollowsTheEvent) {
    TestSharedMemory memory;
    TestEvent event;
    contactci::HapticStateManager manager(memory.name(), event.name());

    EXPECT_FALSE(manager.wait_for_state_change(10));
    event.set();
    EXPECT_TRUE(manager.wait_for_state_change(10));
}

TEST(MutableHapticStateManager, WritesReachSharedMemory) {
    TestSharedMemory memory;
    TestEvent event;
    contactci::MutableHapticStateManager manager(memory.name(), event.name());

    manager.get_left_haptic_state().indexVibrationEffect = 7;
    manager.get_right_haptic_state().littleForceFeedbackPosition = 0.25f;

    EXPECT_EQ(memory.left()->indexVibrationEffect, 7);
    EXPECT_FLOAT_EQ(memory.right()->littleForceFeedbackPosition, 0.25f);
}

#ifdef _WIN32
// signal_haptic_state_changed() sets then immediately resets a manual-reset
// event: anything already waiting is released, and the event ends up reset.
// (On Linux it is one increment of a counter: named_event_posix_test.cpp.)
TEST(MutableHapticStateManager, SignalReleasesAWaiterAndLeavesTheEventReset) {
    TestSharedMemory memory;
    TestEvent event;
    contactci::MutableHapticStateManager manager(memory.name(), event.name());

    std::atomic<bool> woken{false};
    std::thread waiter([&] { woken = WaitForSingleObject(event.handle(), 5000) == WAIT_OBJECT_0; });

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!woken && std::chrono::steady_clock::now() < deadline) {
        manager.signal_haptic_state_changed();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    waiter.join();

    EXPECT_TRUE(woken);
    EXPECT_FALSE(event.is_set());
}
#endif
