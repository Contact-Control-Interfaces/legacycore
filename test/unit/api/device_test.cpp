// Tests that need a real Contact CI service and at least one connected glove -
// the automated counterparts of the tools in test/manual. Without hardware they
// are reported as skipped, so every JUnit report still lists them. On a machine
// with the service running and a glove connected:
//     set CCI_DEVICE_TESTS=1 && .\tools\run-tests.ps1

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "cci/error.h"
#include "cci/session.h"
#include "contactci.h"

namespace {
    bool device_tests_enabled() {
        const char *flag = std::getenv("CCI_DEVICE_TESTS");
        return flag != nullptr && std::string(flag) == "1";
    }
}

class DeviceTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!device_tests_enabled())
            GTEST_SKIP() << "Requires a running Contact CI service and a connected glove "
                            "(the test/manual tools). Set CCI_DEVICE_TESTS=1 to run.";
    }
};

// test/manual/cci_devices, bounded
TEST_F(DeviceTest, CppSessionSeesAConnectedGlove) {
    contactci::Session session;
    bool connected = false;
    for (const auto &device : session.get_device_list()) {
        std::cout << (device.get_is_right() ? "right: " : "left:  ") << device.get_serial_number()
                  << (device.get_is_connected() ? "" : " (disconnected)") << std::endl;
        connected = connected || device.get_is_connected();
    }
    EXPECT_TRUE(connected) << "the service reports no connected glove";
    EXPECT_TRUE(session.get_left_device().has_value() || session.get_right_device().has_value());
}

// test/manual/ccic_devices, bounded
TEST_F(DeviceTest, CSessionSeesAConnectedGlove) {
    CciSessionHandle session = nullptr;
    ASSERT_EQ(cci_create_session(&session), CCI_SUCCESS);

    DeviceListingTransactionHandle transaction = cci_start_device_listing_transaction();
    size_t count = 0;
    EXPECT_EQ(cci_fetch_device_listing(session, transaction, &count), CCI_SUCCESS);
    cci_end_device_listing_transaction(transaction);

    ::DeviceDescription left{}, right{};
    bool any = cci_get_left_device(session, &left) == CCI_SUCCESS ||
               cci_get_right_device(session, &right) == CCI_SUCCESS;
    cci_close_session(session);

    EXPECT_GT(count, 0u);
    EXPECT_TRUE(any) << "no left or right glove connected";
}

// test/manual/cci: ramps the right index vibration for one second, then stops it
TEST_F(DeviceTest, CppMutableSessionDrivesVibration) {
    contactci::MutableHapticSession session;
    auto &state = session.get_session_haptic_state();
    for (int i = 0; i <= 100; ++i) {
        state.get_right_haptic_state().indexVibrationAmplitude = i / 100.0f;
        state.signal_haptic_state_changed();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    state.get_right_haptic_state().indexVibrationAmplitude = 0.0f;
    state.signal_haptic_state_changed();
    EXPECT_FLOAT_EQ(state.get_right_haptic_state().indexVibrationAmplitude, 0.0f);
}

// test/manual/ccic: toggles right index force feedback a few times, then releases it
TEST_F(DeviceTest, CMutableSessionDrivesForceFeedback) {
    CciSessionHandle session = nullptr;
    ASSERT_EQ(cci_create_mutable_haptic_session(&session), CCI_SUCCESS);

    HapticState *left = nullptr, *right = nullptr;
    ASSERT_EQ(cci_get_session_haptic_state(session, &left, &right), CCI_SUCCESS);
    for (int i = 0; i < 6; ++i) {
        right->indexForceFeedbackAmplitude = i % 2 == 0 ? 0.5f : 0.0f;
        EXPECT_EQ(cci_signal_session_haptic_state_changed(session), CCI_SUCCESS);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    right->indexForceFeedbackAmplitude = 0.0f;
    EXPECT_EQ(cci_signal_session_haptic_state_changed(session), CCI_SUCCESS);
    cci_close_session(session);
}
