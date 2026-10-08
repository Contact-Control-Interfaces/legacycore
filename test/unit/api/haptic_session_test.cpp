// HapticSession and MutableHapticSession through the shipped libcci.dll.

#include <atomic>
#include <thread>

#include <gtest/gtest.h>

#include "cci/session.h"
#include "service_fixture.h"

class HapticSessionTest : public WithFakeService {};

TEST_F(HapticSessionTest, ReadOnlySessionAsksForReadAccess) {
    contactci::HapticSession session;

    auto requests = service->session_requests();
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_TRUE(requests[0].isHapticSession);
    EXPECT_FALSE(requests[0].wantsHapticWriteAccess);
}

TEST_F(HapticSessionTest, MutableSessionAsksForWriteAccess) {
    contactci::MutableHapticSession session;

    auto requests = service->session_requests();
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_TRUE(requests[0].isHapticSession);
    EXPECT_TRUE(requests[0].wantsHapticWriteAccess);
}

TEST_F(HapticSessionTest, GlobalStateIsTheServicesMemory) {
    service->globalState.left()->indexForceFeedbackAmplitude = 0.25f;
    service->globalState.right()->ringVibrationAmplitude = 0.8f;

    contactci::HapticSession session;
    const auto &global = session.get_global_haptic_state();

    EXPECT_FLOAT_EQ(global.get_left_haptic_state().indexForceFeedbackAmplitude, 0.25f);
    EXPECT_FLOAT_EQ(global.get_right_haptic_state().ringVibrationAmplitude, 0.8f);
}

TEST_F(HapticSessionTest, WaitsForTheServicesGlobalStateEvent) {
    contactci::HapticSession session;
    const auto &global = session.get_global_haptic_state();

    EXPECT_FALSE(global.wait_for_state_change(10));
    service->globalStateChanged.set();
    EXPECT_TRUE(global.wait_for_state_change(10));
}

TEST_F(HapticSessionTest, SessionStateWritesReachTheService) {
    contactci::MutableHapticSession session;

    session.get_session_haptic_state().get_right_haptic_state().littleVibrationAmplitude = 0.5f;

    EXPECT_FLOAT_EQ(service->sessionState.right()->littleVibrationAmplitude, 0.5f);
    EXPECT_FLOAT_EQ(service->sessionState.left()->littleVibrationAmplitude, 0.0f);
}

TEST_F(HapticSessionTest, SignalWakesTheService) {
    contactci::MutableHapticSession session;

    std::atomic<bool> woken{false};
    std::thread serviceWaiter([&] {
        woken = WaitForSingleObject(service->sessionStateChanged.handle(), 5000) == WAIT_OBJECT_0;
    });
    EXPECT_TRUE(eventually([&] {
        session.get_session_haptic_state().signal_haptic_state_changed();
        return woken.load();
    }));
    serviceWaiter.join();
}
