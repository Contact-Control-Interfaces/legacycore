// contactci::Session through the shipped libcci.dll, against a fake service.

#include <gtest/gtest.h>

#include "cci/error.h"
#include "cci/session.h"
#include "service_fixture.h"

using testsupport::FakeClient;
using testsupport::FakeDevice;

class SessionTest : public WithFakeService {};
class SessionWithoutServiceTest : public WithoutService {};

TEST_F(SessionTest, ReportsTheServicesVersionAndMode) {
    service->set_version("2.0.1");
    service->set_interactive(false);

    contactci::Session session;

    EXPECT_EQ(session.get_service_version(), "2.0.1");
    EXPECT_FALSE(session.is_service_interactive());
    EXPECT_TRUE(session.is_connected());
}

TEST_F(SessionTest, AsksForNoHapticAccess) {
    contactci::Session session;

    auto requests = service->session_requests();
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_FALSE(requests[0].isHapticSession);
    EXPECT_FALSE(requests[0].wantsHapticWriteAccess);
}

TEST_F(SessionTest, ReturnsTheServicesDeviceList) {
    service->set_devices({{"Maestro", "L-1", false, true}, {"Maestro", "R-1", true, false}});

    contactci::Session session;
    auto devices = session.get_device_list();

    ASSERT_EQ(devices.size(), 2u);
    EXPECT_EQ(devices[0].get_product_line(), "Maestro");
    EXPECT_EQ(devices[0].get_serial_number(), "L-1");
    EXPECT_FALSE(devices[0].get_is_right());
    EXPECT_TRUE(devices[0].get_is_connected());
    EXPECT_EQ(devices[1].get_serial_number(), "R-1");
    EXPECT_TRUE(devices[1].get_is_right());
    EXPECT_FALSE(devices[1].get_is_connected());
}

TEST_F(SessionTest, LeftAndRightAreTheConnectedDevices) {
    service->set_devices({
        {"Maestro", "L-1", false, true},
        {"Maestro", "R-old", true, false},   // disconnected: never the right device
        {"Maestro", "R-2", true, true},
    });

    contactci::Session session;
    session.get_device_list();   // waits for the monitor's first fetch

    ASSERT_TRUE(session.get_left_device().has_value());
    EXPECT_EQ(session.get_left_device()->get_serial_number(), "L-1");
    ASSERT_TRUE(session.get_right_device().has_value());
    EXPECT_EQ(session.get_right_device()->get_serial_number(), "R-2");
}

// Regression test: left/right used to come from members the device monitor
// filled on its own thread, so asking right after opening a session could
// race the first fetch and see no device. They now wait for it.
TEST_F(SessionTest, LeftAndRightAreKnownAsSoonAsTheSessionOpens) {
    service->set_devices({{"Maestro", "L-1", false, true}, {"Maestro", "R-1", true, true}});

    for (int i = 0; i < 20; ++i) {
        contactci::Session session;   // no get_device_list() first
        ASSERT_TRUE(session.get_left_device().has_value()) << "iteration " << i;
        EXPECT_EQ(session.get_left_device()->get_serial_number(), "L-1");
        ASSERT_TRUE(session.get_right_device().has_value()) << "iteration " << i;
        EXPECT_EQ(session.get_right_device()->get_serial_number(), "R-1");
    }
}

TEST_F(SessionTest, NoConnectedDevicesMeansNoLeftOrRight) {
    service->set_devices({{"Maestro", "L-1", false, false}});

    contactci::Session session;
    session.get_device_list();

    EXPECT_FALSE(session.get_left_device().has_value());
    EXPECT_FALSE(session.get_right_device().has_value());
}

TEST_F(SessionTest, ReturnsTheServicesClientList) {
    service->set_clients({{100, "Game.exe"}, {200, "Unity.exe"}});

    contactci::Session session;
    auto clients = session.get_client_list();

    ASSERT_EQ(clients.size(), 2u);
    EXPECT_EQ(clients[0].get_process_id(), 100u);
    EXPECT_EQ(clients[0].get_process_name(), "Game.exe");
    EXPECT_EQ(clients[1].get_process_name(), "Unity.exe");
}

TEST_F(SessionTest, RefreshesDevicesWhenTheServiceSignals) {
    contactci::Session session;
    ASSERT_TRUE(session.get_device_list().empty());

    service->set_devices({{"Maestro", "L-9", false, true}});
    service->devicesChanged.set();

    EXPECT_TRUE(eventually([&] { return session.get_device_list().size() == 1; }))
        << "the device monitor did not refetch after devicesChanged was set";
    service->devicesChanged.reset();
}

TEST_F(SessionTest, RefreshesClientsWhenTheServiceSignals) {
    contactci::Session session;
    ASSERT_TRUE(session.get_client_list().empty());

    service->set_clients({{7, "New.exe"}});
    service->clientsChanged.set();

    EXPECT_TRUE(eventually([&] { return session.get_client_list().size() == 1; }));
    service->clientsChanged.reset();
}

// Regression test for the monitor start-up/shutdown race (value_monitor.h):
TEST_F(SessionTest, SurvivesRepeatedOpenAndClose) {
    service->set_devices({{"Maestro", "L-1", false, true}, {"Maestro", "R-1", true, true}});
    for (int i = 0; i < 100; ++i) {
        try {
            contactci::Session session;
            ASSERT_EQ(session.get_device_list().size(), 2u) << "iteration " << i;
        } catch (const contactci::Exception &e) {   // not a std::exception: see error_test.cpp
            FAIL() << "iteration " << i << ": " << get_error_string(e.get_error_code());
        }
    }
}

TEST_F(SessionTest, AccessDeniedThrows) {
    service->set_grant_access(false);

    try {
        contactci::Session session;
        FAIL() << "a session the service refused should throw";
    } catch (const contactci::Exception &e) {
        EXPECT_EQ(e.get_error_code(), CCI_ERR_SESSION_ACCESS_DENIED);
    }
}

TEST_F(SessionWithoutServiceTest, ThrowsPipeFailedToOpen) {
    try {
        contactci::Session session;
        FAIL() << "a session without a service should throw";
    } catch (const contactci::Exception &e) {
        EXPECT_EQ(e.get_error_code(), CCI_ERR_PIPE_FAILED_TO_OPEN);
    }
}
