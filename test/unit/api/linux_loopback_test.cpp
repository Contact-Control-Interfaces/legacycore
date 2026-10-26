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
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include <contactci/haptic_state.h>

#include "contactci.h"
#include "cci/error.h"
#include "cci/session.h"
#include "service_fixture.h"

using namespace std::chrono_literals;
using testsupport::SessionAttachments;

namespace {
    using Clock = std::chrono::steady_clock;

    // Every descriptor this process holds, the daemon's included: Core must close each one the daemon passes it,
    // on every path. `prefix` counts only those whose target starts with it.
    int open_descriptors(const std::string &prefix = "") {
        int count = 0;
        for (const auto &entry : std::filesystem::directory_iterator("/proc/self/fd")) {
            std::error_code gone;   // the iterator's own descriptor, or one closed meanwhile
            std::string target = std::filesystem::read_symlink(entry.path(), gone).string();
            if (!gone && target.rfind(prefix, 0) == 0)
                ++count;
        }
        return count;
    }

    // How many lines of /proc/self/maps name a file whose path starts with `prefix`: the daemon's own mappings
    // included, so a test compares counts before and after.
    int mappings_of_files(const std::string &prefix) {
        int count = 0;
        std::ifstream maps("/proc/self/maps");
        for (std::string line; std::getline(maps, line);) {
            std::size_t path = line.find('/');
            if (path != std::string::npos && line.compare(path, prefix.size(), prefix) == 0)
                ++count;
        }
        return count;
    }

    // What a failed Session must leave behind: nothing. Every descriptor Core was passed is closed, eventually
    // (the daemon closes its end of the connection on its own thread), and Core maps no object, by name or passed.
    struct NothingLeftOpen {
        int descriptors = open_descriptors();
        int namedMappings = mappings_of_files("/dev/shm/");
        int memfdMappings = mappings_of_files("/memfd:");

        void check() const {
            auto deadline = Clock::now() + 5s;
            while (open_descriptors() != descriptors && Clock::now() < deadline)
                std::this_thread::sleep_for(10ms);
            EXPECT_EQ(open_descriptors(), descriptors) << "descriptor(s) left open";
            EXPECT_EQ(mappings_of_files("/dev/shm/"), namedMappings) << "a named object is still mapped";
            EXPECT_EQ(mappings_of_files("/memfd:"), memfdMappings) << "a passed object is still mapped";
        }
    };

    // The line of /proc/self/maps for the mapping that holds `address`.
    std::string mapping_of(const void *address) {
        auto at = reinterpret_cast<std::uintptr_t>(address);
        std::ifstream maps("/proc/self/maps");
        for (std::string line; std::getline(maps, line);) {
            std::uintptr_t start = 0, end = 0;
            char dash = 0;
            std::istringstream fields(line);
            fields >> std::hex >> start >> dash >> end;
            if (at >= start && at < end)
                return line;
        }
        return "";
    }

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

// §3: Core asks for protocol version 1, and a write Session's own region and
// counter arrive as descriptors: Core maps the daemon's sealed memfds, opens
// nothing by name for them, and keeps no descriptor once they are mapped.
TEST_F(LinuxLoopbackTest, AWriteSessionMapsTheDescriptorsTheDaemonPassesAndKeepsNone) {
    int memfdsBefore = open_descriptors("/memfd:");   // the daemon's own two
    int namedBefore = mappings_of_files("/dev/shm/");
    contactci::MutableHapticSession session;
    auto &mine = session.get_session_haptic_state();

    auto requests = service->session_requests();
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_EQ(requests[0].protocolVersion, static_cast<uint32_t>(CONTACTCI_PROTOCOL_VERSION));

    std::string mapping = mapping_of(&mine.get_left_haptic_state());
    EXPECT_NE(mapping.find("/memfd:cci-test-session"), std::string::npos) << mapping;
    EXPECT_NE(mapping.find(" rw-s "), std::string::npos) << "not a shared, writable mapping: " << mapping;

    EXPECT_EQ(open_descriptors("/memfd:"), memfdsBefore) << "Core kept a passed descriptor open after mapping it";

    // Nothing in /dev/shm for the Session: the only named objects Core maps are the four global ones (§4). Counted
    // in this process's mappings rather than by listing /dev/shm, which other test processes write to meanwhile
    EXPECT_EQ(mappings_of_files("/dev/shm/") - namedBefore, 4);

    mine.get_left_haptic_state().thumbForceFeedbackAmplitude = 0.25f;
    mine.signal_haptic_state_changed();
    EXPECT_TRUE(service->sessionStateChanged.wait(5000));
    EXPECT_FLOAT_EQ(service->sessionState.left()->thumbForceFeedbackAmplitude, 0.25f);
}

// §3: the descriptors travel with the first byte of the frame, which may come
// on its own; Core reads on to the whole frame.
TEST_F(LinuxLoopbackTest, AReplySplitAfterItsFirstByteStillCarriesTheDescriptors) {
    service->set_split_session_reply(true);

    contactci::MutableHapticSession session;
    session.get_session_haptic_state().get_right_haptic_state().littleVibrationEffect = 7;

    EXPECT_EQ(service->sessionState.right()->littleVibrationEffect, 7);
}

// §3: exactly two descriptors, of exactly the contract sizes, or the Session
// fails with CCI_ERR_SHM_FAILED_TO_OPEN, and every descriptor that came is
// closed. Core has room for two, so a third is dropped by the kernel and only
// MSG_CTRUNC says it was sent.
TEST_F(LinuxLoopbackTest, AWriteSessionWithTheWrongDescriptorsFailsAndClosesThemAll) {
    struct Case {
        SessionAttachments attachments;
        const char *what;
    };
    for (Case broken : {Case{SessionAttachments::Nothing, "no descriptors"},
                        Case{SessionAttachments::RegionOnly, "one descriptor"},
                        Case{SessionAttachments::OneTooMany, "three descriptors, truncated to two (MSG_CTRUNC)"},
                        Case{SessionAttachments::ShortRegion, "a 72-byte region"},
                        Case{SessionAttachments::LongCounter, "an 8-byte counter"}}) {
        SCOPED_TRACE(broken.what);
        service->set_session_attachments(broken.attachments);
        NothingLeftOpen nothing;

        EXPECT_EQ(status_of([] { contactci::MutableHapticSession session; }), CCI_ERR_SHM_FAILED_TO_OPEN);

        nothing.check();
    }
}

// §3: the write fields hold fd:0 and fd:1 when the objects come as
// descriptors. Real names next to them mean a Service that still creates named
// write objects too, which any contactci member could open (DEV-106).
TEST_F(LinuxLoopbackTest, AWriteSessionWhoseReplyStillNamesItsObjectsFails) {
    for (auto names : {std::pair{"/contactci-haptic-memory-0123", "fd:1"},
                       std::pair{"fd:0", "/contactci-event-haptics-0123"},
                       std::pair{"", ""}}) {
        SCOPED_TRACE(std::string(names.first) + ", " + names.second);
        service->set_write_field_names(names.first, names.second);
        NothingLeftOpen nothing;

        EXPECT_EQ(status_of([] { contactci::MutableHapticSession session; }), CCI_ERR_SHM_FAILED_TO_OPEN);

        nothing.check();
    }
}

// §3: a Session without write access expects no descriptors, and fails if any come.
TEST_F(LinuxLoopbackTest, DescriptorsOnASessionWithoutWriteAccessFailIt) {
    service->set_session_attachments(SessionAttachments::EvenWithoutWriteAccess);
    NothingLeftOpen nothing;

    EXPECT_EQ(status_of([] { contactci::Session session; }), CCI_ERR_SHM_FAILED_TO_OPEN);
    EXPECT_EQ(status_of([] { contactci::HapticSession session; }), CCI_ERR_SHM_FAILED_TO_OPEN);
    EXPECT_EQ(status_of([] { contactci::MutableHapticSession session; }), CCI_SUCCESS);

    nothing.check();
}

// §3: Core closes any descriptor that arrives on a frame other than opcode 45.
TEST_F(LinuxLoopbackTest, ADescriptorOnALaterFrameIsClosed) {
    contactci::Session session;
    session.get_device_list();   // the device monitor's first fetch is done
    service->set_descriptor_on_device_lists(true);
    int before = open_descriptors();

    for (int fetch = 1; fetch <= 3; ++fetch) {
        int requests = service->device_list_requests();
        service->devicesChanged.set();
        ASSERT_TRUE(eventually([&] { return service->device_list_requests() > requests; }));
    }

    EXPECT_TRUE(eventually([&] { return open_descriptors() == before; }))
        << open_descriptors() - before << " descriptor(s) left open";
    EXPECT_TRUE(session.is_connected());
}

// §3, "Protocol version": a reply with any version but the one Core asked for
// is a refusal. Core fails the Session, opens nothing and closes the
// connection. 0, a Service from before the field, is refused too on Linux. A
// Service on another version may attach descriptors anyway; Core closes them.
TEST_F(LinuxLoopbackTest, AServiceOnAnotherProtocolVersionIsRefused) {
    for (auto attachments : {SessionAttachments::AsContract, SessionAttachments::EvenWithoutWriteAccess}) {
        for (uint32_t version : {0u, static_cast<uint32_t>(CONTACTCI_PROTOCOL_VERSION) + 1}) {
            SCOPED_TRACE("protocol version " + std::to_string(version) +
                         (attachments == SessionAttachments::AsContract ? ", no descriptors" : ", with descriptors"));
            service->set_session_attachments(attachments);
            service->set_protocol_version(version);
            NothingLeftOpen nothing;

            EXPECT_EQ(status_of([] { contactci::MutableHapticSession session; }), CCI_ERR_SESSION_PROTOCOL_MISMATCH);
            EXPECT_EQ(status_of([] { contactci::Session session; }), CCI_ERR_SESSION_PROTOCOL_MISMATCH);

            EXPECT_TRUE(eventually([&] { return service->connections_open() == 0; }));
            nothing.check();
        }
    }
}

// The code the Manager maps to "Core and Service versions don't match" (DEV-146).
TEST_F(LinuxLoopbackTest, TheCApiReportsAProtocolMismatch) {
    service->set_protocol_version(CONTACTCI_PROTOCOL_VERSION + 1);
    CciSessionHandle handle = nullptr;

    EXPECT_EQ(cci_create_mutable_haptic_session(&handle), CCI_ERR_SESSION_PROTOCOL_MISMATCH);
    EXPECT_EQ(handle, nullptr);
}

// §3: a reply that isn't opcode 45, or doesn't parse, is a broken Service, not a refusal.
// The daemon still attaches the write Session's descriptors to these replies; Core closes them.
TEST_F(LinuxLoopbackTest, ABrokenSessionReplyFailsToRead) {
    {
        SCOPED_TRACE("another opcode");
        service->set_session_reply_opcode(OpCode::opDeviceListResponseMessage);
        NothingLeftOpen nothing;
        EXPECT_EQ(status_of([] { contactci::MutableHapticSession session; }), CCI_ERR_PIPE_FAILED_TO_READ);
        nothing.check();
    }
    {
        SCOPED_TRACE("a payload that does not parse");
        service->set_session_reply_opcode(OpCode::opSessionInitializationResponseMessage);
        service->set_unparseable_session_reply(true);
        NothingLeftOpen nothing;
        EXPECT_EQ(status_of([] { contactci::MutableHapticSession session; }), CCI_ERR_PIPE_FAILED_TO_READ);
        nothing.check();
    }
}

// NamedPipe refuses a path that doesn't fit sockaddr_un.sun_path (108 bytes with its null).
TEST(LinuxSocketPath, AnOverrideTooLongForSunPathFailsToOpen) {
    testsupport::ScopedEnvironment tooLong(testsupport::ServiceSocketVariable, "/tmp/" + std::string(200, 'x'));

    EXPECT_EQ(status_of([] { contactci::Session session; }), CCI_ERR_PIPE_FAILED_TO_OPEN);
}
