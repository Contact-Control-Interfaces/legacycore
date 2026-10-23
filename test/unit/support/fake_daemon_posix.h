#pragma once

// A stand-in for the Contact CI Linux Service (linux-service), for the loopback
// tests: no real Service or glove is needed. It follows
// communication/docs/linux-core-service-contract.md, written from the contract
// rather than from Core's code:
//
//   §1  it listens on an AF_UNIX stream socket, in a fresh temporary directory
//       because a CI runner can't create /run/contactci, and points Core at it
//       with CONTACTCI_SERVICE_SOCKET for its lifetime
//   §2  it reads and writes 10-byte PacketHeaders followed by the payload
//   §3  it answers session init (opcode 44) with opcode 45: protocol version 1,
//       the four global names, and for a write Session the placeholders fd:0
//       and fd:1, with the Session's region and counter attached as
//       descriptors (SCM_RIGHTS). It records a connection whose first message
//       is anything else, or that asks for another protocol version, which it
//       refuses and closes as the Service does
//   §4  it creates the global objects by name before it answers, and unlinks
//       them all; the Session's objects are sealed memfds with no name
//   §5  the haptic state regions are 144 bytes
//   §6  every change signal is a futex sequence counter (posix_objects.h)
//
// Like the real Service, one connection is one Session, served on its own
// thread. Unlike it, every write Session is passed the same region and counter,
// sessionState and sessionStateChanged, so a test can reach them the same way
// on both platforms. It has the same members as the Windows FakeService
// (fake_service.h), so the platform-neutral API tests run against either;
// FakeService names it.

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <set>
#include <vector>
#include <cstdint>

#include "posix_objects.h"

namespace testsupport {
    struct FakeDevice {
        std::string productLine;
        std::string serialNumber;
        bool isRight;
        bool isConnected;
    };

    struct FakeClient {
        uint32_t processId;
        std::string processName;
    };

    struct SessionRequest {
        bool isHapticSession;
        bool wantsHapticWriteAccess;
        uint32_t protocolVersion;
    };

    // What a session response carries as descriptors. The contract (§3) is the
    // Session's region and counter, in that order, when write access is
    // granted, and nothing otherwise; every other value breaks it on purpose.
    enum class SessionAttachments {
        AsContract,
        Nothing,                  // write access granted, but no descriptors
        RegionOnly,               // one descriptor instead of two
        OneTooMany,               // a third descriptor after the two
        ShortRegion,              // a region of one HapticState, 72 bytes
        LongCounter,              // a counter of 8 bytes
        EvenWithoutWriteAccess,   // the two descriptors on every response
    };

    // Core's override of the compiled-in /run/contactci/service.sock (pipe_channel_posix.cpp).
    constexpr const char *ServiceSocketVariable = "CONTACTCI_SERVICE_SOCKET";

    // A socket path in a directory that does not exist: nothing listens there.
    std::string missing_socket_path();

    // Sets an environment variable for its lifetime, then puts back what was there.
    class ScopedEnvironment {
    public:
        ScopedEnvironment(const char *name, const std::string &value);
        ~ScopedEnvironment();
        ScopedEnvironment(const ScopedEnvironment &) = delete;
        ScopedEnvironment &operator=(const ScopedEnvironment &) = delete;

    private:
        std::string name_;
        bool hadValue_;
        std::string oldValue_;
    };

    class FakeDaemon {
    public:
        FakeDaemon();
        ~FakeDaemon();
        FakeDaemon(const FakeDaemon &) = delete;
        FakeDaemon &operator=(const FakeDaemon &) = delete;

        const std::string &socket_path() const { return socketPath_; }

        // What the next session response says.
        void set_version(const std::string &version);
        void set_interactive(bool interactive);
        void set_grant_access(bool grant);

        // The contract version the next session responses carry: 1, this
        // contract, by default. Another one is a refusal, and the daemon closes
        // the connection after sending it, as the Service does.
        void set_protocol_version(uint32_t version);

        // Break the next session responses as a broken Service would: another
        // opcode than 45, or a payload that does not parse.
        void set_session_reply_opcode(int opcode);
        void set_unparseable_session_reply(bool unparseable);

        void set_session_attachments(SessionAttachments attachments);

        // Send a session response's first byte, with its descriptors, on its
        // own and then the rest, so Core has to read on to the whole frame.
        void set_split_session_reply(bool split);

        // Attach a descriptor to every device list response, which the contract
        // never does; Core must close it.
        void set_descriptor_on_device_lists(bool attach);

        // Close the client's connection instead of answering a device list request,
        // as a Service that disconnects mid-fetch does.
        void set_drop_on_device_list(bool drop);

        // What device/client list requests return; may change while a session is open.
        void set_devices(const std::vector<FakeDevice> &devices);
        void set_clients(const std::vector<FakeClient> &clients);

        // Hang up on every open connection, as a Service that stops does.
        void disconnect_all();

        // What clients sent.
        std::vector<SessionRequest> session_requests() const;
        int device_list_requests() const;
        int client_list_requests() const;
        int connections_accepted() const;
        int connections_open() const;      // until the daemon sees the client close it
        std::vector<std::string> protocol_errors() const;

        // The objects a session response points the client at.
        TestEvent devicesChanged;
        TestEvent clientsChanged;
        TestEvent globalStateChanged;    // hapticReadEventName
        TestEvent sessionStateChanged{ObjectKind::Sealed};                          // descriptor [1]
        TestSharedMemory globalState;                                                // hapticReadSharedMemoryName
        TestSharedMemory sessionState{2 * sizeof(HapticState), ObjectKind::Sealed};  // descriptor [0]

    private:
        void accept_loop();
        void serve(int connection);

        std::string directory_;
        std::string socketPath_;
        int listener_ = -1;
        int wake_[2] = {-1, -1};          // written once by the destructor to stop accept_loop
        std::unique_ptr<ScopedEnvironment> pointCoreHere_;
        std::thread acceptThread_;

        mutable std::mutex connectionsLock_;
        bool stopping_ = false;
        std::set<int> openConnections_;
        std::vector<std::thread> clientThreads_;
        int connectionsAccepted_ = 0;

        mutable std::mutex lock_;
        std::string version_ = "9.9.9-test";
        bool interactive_ = true;
        bool grantAccess_ = true;
        bool dropOnDeviceList_ = false;
        uint32_t protocolVersion_ = CONTACTCI_PROTOCOL_VERSION;
        int sessionReplyOpcode_ = 45;   // opSessionInitializationResponseMessage
        bool unparseableSessionReply_ = false;
        SessionAttachments sessionAttachments_ = SessionAttachments::AsContract;
        bool splitSessionReply_ = false;
        bool descriptorOnDeviceLists_ = false;
        std::vector<FakeDevice> devices_;
        std::vector<FakeClient> clients_;
        std::vector<SessionRequest> sessionRequests_;
        int deviceListRequests_ = 0;
        int clientListRequests_ = 0;
        std::vector<std::string> protocolErrors_;
    };

    using FakeService = FakeDaemon;
}
