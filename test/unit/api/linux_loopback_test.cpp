// Linux only: the loopback test (DEV-19). Core's Linux transport end to end
// through the public API of the shipped libcci.so, against the fake daemon
// (support/fake_daemon_posix.h): the Unix socket (PipeChannel, NamedPipe), the
// haptic state regions (SharedMemoryManager) and the futex counters
// (NamedEvent), with nothing inside Core replaced. The platform-neutral API
// tests (session_test.cpp, haptic_session_test.cpp, c_api_test.cpp) run against
// the same daemon; these cover what is particular to the Linux contract
// (communication/docs/linux-core-service-contract.md). Every test also fails if
// the daemon saw Core break the contract (WithFakeService::TearDown).

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "cci/error.h"
#include "cci/session.h"
#include "service_fixture.h"

using namespace std::chrono_literals;

namespace {
    using Clock = std::chrono::steady_clock;

    template <typename Action>
    CciStatus status_of(Action action) {
        try {
            action();
        } catch (const contactci::Exception &e) {
            return e.get_error_code();
        }
        return CCI_SUCCESS;
    }
}

class LinuxLoopbackTest : public WithFakeService {};

// §1: CONTACTCI_SERVICE_SOCKET replaces /run/contactci/service.sock.
TEST_F(LinuxLoopbackTest, CoreConnectsToTheSocketTheOverrideNames) {
    ASSERT_STREQ(std::getenv(testsupport::ServiceSocketVariable), service->socket_path().c_str());

    contactci::Session session;

    EXPECT_EQ(service->connections_accepted(), 1);
    ASSERT_EQ(service->session_requests().size(), 1u);
    EXPECT_TRUE(session.is_connected());
}

// §1: one connection is one Session, and the Session ends when it closes.
TEST_F(LinuxLoopbackTest, EachSessionIsItsOwnConnectionAndClosingItHangsUp) {
    {
        contactci::Session plain;
        contactci::HapticSession readOnly;
        contactci::MutableHapticSession mutableSession;

        EXPECT_EQ(service->connections_accepted(), 3);
        EXPECT_EQ(service->connections_open(), 3);
    }
    EXPECT_TRUE(eventually([&] { return service->connections_open() == 0; }))
        << service->connections_open() << " connection(s) still open after every Session closed";
}

// §3: session init first (opcode 44, answered with 45), then the list requests on the same connection.
TEST_F(LinuxLoopbackTest, SessionInitComesFirstThenTheListsOnTheSameConnection) {
    service->set_devices({{"Maestro", "L-1", false, true}});
    service->set_clients({{42, "client"}});

    contactci::MutableHapticSession session;
    EXPECT_EQ(session.get_device_list().size(), 1u);
    EXPECT_EQ(session.get_client_list().size(), 1u);

    EXPECT_EQ(service->connections_accepted(), 1);
    auto requests = service->session_requests();
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_TRUE(requests[0].isHapticSession);
    EXPECT_TRUE(requests[0].wantsHapticWriteAccess);
    EXPECT_GE(service->device_list_requests(), 1);
    EXPECT_GE(service->client_list_requests(), 1);
}

TEST_F(LinuxLoopbackTest, IsConnectedTurnsFalseWhenTheDaemonHangsUp) {
    contactci::Session session;
    ASSERT_TRUE(session.is_connected());

    service->disconnect_all();

    EXPECT_TRUE(eventually([&] { return !session.is_connected(); }));
}

// Writing to a socket the Service closed raises SIGPIPE unless Core asks it not
// to (MSG_NOSIGNAL); that would kill this whole test process.
TEST_F(LinuxLoopbackTest, ARequestAfterTheDaemonHungUpFailsInsteadOfKillingTheClient) {
    contactci::Session session;
    session.get_device_list();   // the device monitor's first fetch is done
    service->disconnect_all();
    ASSERT_TRUE(eventually([&] { return !session.is_connected(); }));

    service->devicesChanged.set();   // the monitor refetches over the dead socket

    CciStatus status = CCI_SUCCESS;
    EXPECT_TRUE(eventually([&] {
        status = status_of([&] { session.get_device_list(); });
        return status != CCI_SUCCESS;
    })) << "the refetch over the closed socket never failed";
    EXPECT_TRUE(status == CCI_ERR_PIPE_FAILED_TO_WRITE || status == CCI_ERR_PIPE_FAILED_TO_READ)
        << get_error_string(status);
}

// §5 and §6: the Service writes the Global haptic state, then bumps its
// counter; Core, blocked on a read-only mapping of that counter, wakes and
// re-reads the region.
TEST_F(LinuxLoopbackTest, AGlobalStateWaitIsWokenByTheDaemonAndSeesItsWrite) {
    contactci::HapticSession session;
    const auto &global = session.get_global_haptic_state();

    auto start = Clock::now();
    std::thread daemon([&] {
        std::this_thread::sleep_for(200ms);
        service->globalState.right()->ringVibrationAmplitude = 0.5f;
        service->globalStateChanged.set();
    });
    bool changed = global.wait_for_state_change(10000);
    auto elapsed = Clock::now() - start;
    daemon.join();

    EXPECT_TRUE(changed);
    EXPECT_GE(elapsed, 200ms) << "returned before the daemon signalled";
    EXPECT_LT(elapsed, 5s) << "only the timeout ended the wait: the daemon's FUTEX_WAKE did not reach Core";
    EXPECT_FLOAT_EQ(global.get_right_haptic_state().ringVibrationAmplitude, 0.5f);
}

// ADR-0001: unlike the Windows pulse, changes made while the Client is busy are not lost.
TEST_F(LinuxLoopbackTest, GlobalChangesMadeWhileTheClientIsBusyAreNotLost) {
    contactci::HapticSession session;
    const auto &global = session.get_global_haptic_state();

    for (int i = 1; i <= 3; ++i) {
        service->globalState.left()->thumbVibrationAmplitude = i / 10.0f;
        service->globalStateChanged.set();
    }

    EXPECT_TRUE(global.wait_for_state_change(0)) << "a change made before the wait was lost";
    EXPECT_FLOAT_EQ(global.get_left_haptic_state().thumbVibrationAmplitude, 0.3f);
    EXPECT_FALSE(global.wait_for_state_change(10)) << "nothing should be left pending after one wake";
}

// §6: Core's signal on the Session haptic state is one increment and a shared FUTEX_WAKE.
TEST_F(LinuxLoopbackTest, ASessionSignalIsOneIncrementThatWakesTheDaemon) {
    contactci::MutableHapticSession session;
    auto &state = session.get_session_haptic_state();

    std::atomic<bool> woken{false};
    auto start = Clock::now();
    std::thread daemon([&] { woken = service->sessionStateChanged.wait(10000); });
    std::this_thread::sleep_for(200ms);
    state.get_right_haptic_state().indexForceFeedbackAmplitude = 0.7f;
    state.signal_haptic_state_changed();
    daemon.join();

    EXPECT_TRUE(woken);
    EXPECT_LT(Clock::now() - start, 5s) << "only the timeout ended the daemon's wait";
    EXPECT_EQ(service->sessionStateChanged.count(), 1u);
    EXPECT_FLOAT_EQ(service->sessionState.right()->indexForceFeedbackAmplitude, 0.7f);

    state.signal_haptic_state_changed();
    EXPECT_EQ(service->sessionStateChanged.count(), 2u);
}

// §5: both regions are 144 bytes, left at offset 0 and right at 72, shared byte for byte.
TEST_F(LinuxLoopbackTest, BothHapticRegionsAre144BytesSharedByteForByte) {
    ASSERT_EQ(service->globalState.size(), 144u);
    ASSERT_EQ(service->sessionState.size(), 144u);
    contactci::MutableHapticSession session;

    auto *daemonGlobal = static_cast<unsigned char *>(service->globalState.data());
    for (std::size_t i = 0; i < 144; ++i)
        daemonGlobal[i] = static_cast<unsigned char>(i);
    const auto &global = session.get_global_haptic_state();
    auto *coreGlobal = reinterpret_cast<const unsigned char *>(&global.get_left_haptic_state());
    EXPECT_EQ(reinterpret_cast<const unsigned char *>(&global.get_right_haptic_state()), coreGlobal + 72);
    for (std::size_t i = 0; i < 144; ++i)
        ASSERT_EQ(coreGlobal[i], i) << "Global haptic state byte " << i;

    auto &mine = session.get_session_haptic_state();
    auto *coreSession = reinterpret_cast<unsigned char *>(&mine.get_left_haptic_state());
    EXPECT_EQ(reinterpret_cast<unsigned char *>(&mine.get_right_haptic_state()), coreSession + 72);
    for (std::size_t i = 0; i < 144; ++i)
        coreSession[i] = static_cast<unsigned char>(255 - i);
    auto *daemonSession = static_cast<unsigned char *>(service->sessionState.data());
    for (std::size_t i = 0; i < 144; ++i)
        ASSERT_EQ(daemonSession[i], 255 - i) << "Session haptic state byte " << i;
}

// NamedPipe refuses a path that doesn't fit sockaddr_un.sun_path (108 bytes with its null).
TEST(LinuxSocketPath, AnOverrideTooLongForSunPathFailsToOpen) {
    testsupport::ScopedEnvironment tooLong(testsupport::ServiceSocketVariable, "/tmp/" + std::string(200, 'x'));

    EXPECT_EQ(status_of([] { contactci::Session session; }), CCI_ERR_PIPE_FAILED_TO_OPEN);
}
