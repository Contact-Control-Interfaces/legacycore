// The C API in the shipped libccic.dll - what CoreConductor P/Invokes and C
// clients call - against a fake service.

#include <vector>

#include <gtest/gtest.h>

#include "contactci.h"
#include "service_fixture.h"

class CApiTest : public WithFakeService {
protected:
    // Owns one session handle and always closes it, even when an assertion fails.
    struct Session {
        CciSessionHandle handle = nullptr;
        ~Session() { if (handle) cci_close_session(handle); }
    };
};
class CApiWithoutServiceTest : public WithoutService {};

TEST_F(CApiTest, CreateSessionReportsTheService) {
    service->set_version("2.3.12-test");
    Session s;
    ASSERT_EQ(cci_create_session(&s.handle), CCI_SUCCESS);

    EXPECT_STREQ(cci_get_session_service_version(s.handle), "2.3.12-test");
    EXPECT_TRUE(cci_is_session_service_interactive(s.handle));
    EXPECT_EQ(cci_is_session_connected(s.handle), CCI_SUCCESS);
}

TEST_F(CApiTest, DeviceListingTransaction) {
    service->set_devices({{"Maestro", "L-1", false, true}, {"Maestro", "R-1", true, true}});
    Session s;
    ASSERT_EQ(cci_create_session(&s.handle), CCI_SUCCESS);

    DeviceListingTransactionHandle transaction = cci_start_device_listing_transaction();
    size_t count = 0;
    ASSERT_EQ(cci_fetch_device_listing(s.handle, transaction, &count), CCI_SUCCESS);
    ASSERT_EQ(count, 2u);
    std::vector<DeviceDescription> devices(count);
    cci_get_device_listing(transaction, devices.data());

    EXPECT_STREQ(devices[0].productLine, "Maestro");
    EXPECT_STREQ(devices[0].serialNumber, "L-1");
    EXPECT_FALSE(devices[0].isRight);
    EXPECT_STREQ(devices[1].serialNumber, "R-1");
    EXPECT_TRUE(devices[1].isRight);
    EXPECT_TRUE(devices[1].isConnected);
    cci_end_device_listing_transaction(transaction);
}

TEST_F(CApiTest, ClientListingTransaction) {
    service->set_clients({{321, "Game.exe"}});
    Session s;
    ASSERT_EQ(cci_create_session(&s.handle), CCI_SUCCESS);

    ClientListingTransactionHandle transaction = cci_start_client_listing_transaction();
    size_t count = 0;
    ASSERT_EQ(cci_fetch_client_listing(s.handle, transaction, &count), CCI_SUCCESS);
    ASSERT_EQ(count, 1u);
    ClientDescription client{};
    cci_get_client_listing(transaction, &client);

    EXPECT_EQ(client.processID, 321u);
    EXPECT_STREQ(client.processName, "Game.exe");
    cci_end_client_listing_transaction(transaction);
}

TEST_F(CApiTest, LeftAndRightDevice) {
    service->set_devices({{"Maestro", "L-1", false, true}});
    Session s;
    ASSERT_EQ(cci_create_session(&s.handle), CCI_SUCCESS);
    // Fetch once first: left/right come from the device monitor's first fetch.
    DeviceListingTransactionHandle transaction = cci_start_device_listing_transaction();
    size_t count = 0;
    ASSERT_EQ(cci_fetch_device_listing(s.handle, transaction, &count), CCI_SUCCESS);
    cci_end_device_listing_transaction(transaction);

    DeviceDescription left{};
    ASSERT_EQ(cci_get_left_device(s.handle, &left), CCI_SUCCESS);
    EXPECT_STREQ(left.serialNumber, "L-1");
    EXPECT_EQ(cci_get_left_device(s.handle, nullptr), CCI_SUCCESS) << "out may be null";

    DeviceDescription right{};
    EXPECT_EQ(cci_get_right_device(s.handle, &right), CCI_NO_DEVICE);
}

TEST_F(CApiTest, PlainSessionHasNoHapticState) {
    Session s;
    ASSERT_EQ(cci_create_session(&s.handle), CCI_SUCCESS);

    HapticState *left = nullptr, *right = nullptr;
    const HapticState *globalLeft = nullptr, *globalRight = nullptr;
    bool changed = false;
    EXPECT_EQ(cci_get_session_haptic_state(s.handle, &left, &right), CCI_ERR_SESSION_INVALID_HANDLE);
    EXPECT_EQ(cci_get_global_haptic_state(s.handle, &globalLeft, &globalRight), CCI_ERR_SESSION_INVALID_HANDLE);
    EXPECT_EQ(cci_signal_session_haptic_state_changed(s.handle), CCI_ERR_SESSION_INVALID_HANDLE);
    EXPECT_EQ(cci_wait_global_haptic_state_changed(s.handle, 10, &changed), CCI_ERR_SESSION_INVALID_HANDLE);
}

TEST_F(CApiTest, ReadOnlyHapticSessionReadsButCannotWrite) {
    service->globalState.left()->thumbVibrationAmplitude = 0.6f;
    Session s;
    ASSERT_EQ(cci_create_readonly_haptic_session(&s.handle), CCI_SUCCESS);

    const HapticState *left = nullptr, *right = nullptr;
    ASSERT_EQ(cci_get_global_haptic_state(s.handle, &left, &right), CCI_SUCCESS);
    EXPECT_FLOAT_EQ(left->thumbVibrationAmplitude, 0.6f);
    EXPECT_EQ(right, left + 1);

    HapticState *writeLeft = nullptr, *writeRight = nullptr;
    EXPECT_EQ(cci_get_session_haptic_state(s.handle, &writeLeft, &writeRight), CCI_ERR_SESSION_INVALID_HANDLE);
}

TEST_F(CApiTest, WaitGlobalHapticStateChanged) {
    Session s;
    ASSERT_EQ(cci_create_readonly_haptic_session(&s.handle), CCI_SUCCESS);

    bool changed = true;
    ASSERT_EQ(cci_wait_global_haptic_state_changed(s.handle, 10, &changed), CCI_SUCCESS);
    EXPECT_FALSE(changed);

    service->globalStateChanged.set();
    ASSERT_EQ(cci_wait_global_haptic_state_changed(s.handle, 10, &changed), CCI_SUCCESS);
    EXPECT_TRUE(changed);
}

TEST_F(CApiTest, MutableSessionWritesAndSignals) {
    Session s;
    ASSERT_EQ(cci_create_mutable_haptic_session(&s.handle), CCI_SUCCESS);

    HapticState *left = nullptr, *right = nullptr;
    ASSERT_EQ(cci_get_session_haptic_state(s.handle, &left, &right), CCI_SUCCESS);
    right->indexForceFeedbackAmplitude = 0.3f;
    left->middleVibrationEffect = 4;

    EXPECT_FLOAT_EQ(service->sessionState.right()->indexForceFeedbackAmplitude, 0.3f);
    EXPECT_EQ(service->sessionState.left()->middleVibrationEffect, 4);
    EXPECT_EQ(cci_signal_session_haptic_state_changed(s.handle), CCI_SUCCESS);
}

TEST_F(CApiTest, AccessDeniedReturnsItsStatus) {
    service->set_grant_access(false);
    CciSessionHandle handle = nullptr;

    EXPECT_EQ(cci_create_mutable_haptic_session(&handle), CCI_ERR_SESSION_ACCESS_DENIED);
    EXPECT_EQ(handle, nullptr);
}

TEST_F(CApiWithoutServiceTest, CreateSessionReturnsPipeFailedToOpen) {
    CciSessionHandle handle = nullptr;
    EXPECT_EQ(cci_create_session(&handle), CCI_ERR_PIPE_FAILED_TO_OPEN);
    EXPECT_EQ(handle, nullptr);
}

TEST(CApiErrorStrings, EveryStatusHasAMessage) {
    EXPECT_STREQ(cci_get_error_string(CCI_SUCCESS), "Success");
    EXPECT_STREQ(cci_get_error_string(CCI_ERR_SESSION_INVALID_HANDLE), "Invalid session handle");
    for (int status = CCI_SUCCESS; status <= CCI_ERR_SHM_FAILED_TO_MAP; ++status)
        EXPECT_NE(cci_get_error_string(static_cast<CciStatus>(status)), nullptr) << "status " << status;
    EXPECT_EQ(cci_get_error_string(static_cast<CciStatus>(CCI_ERR_SHM_FAILED_TO_MAP + 1)), nullptr);
}
