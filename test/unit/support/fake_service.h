#pragma once

// A stand-in for the Contact CI Windows service. It owns the service's pipe,
// \\.\pipe\contact-ci-service, answers the requests libcci sends (session
// initialisation, device list, client list) with the protocol's framing, and
// creates the events and shared memory a session response names.
//
// Like the real service it always keeps a free pipe instance listening and
// serves each client on its own thread, so a session can open the moment the
// previous one closed. If a real service is running, its pipe already exists;
// tests check real_service_running() and skip rather than fight it for the name.

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>

#include "win_objects.h"

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
    };

    class FakeService {
    public:
        static constexpr const char *PipeName = R"(\\.\pipe\contact-ci-service)";
        static bool real_service_running();

        FakeService();
        ~FakeService();
        FakeService(const FakeService &) = delete;
        FakeService &operator=(const FakeService &) = delete;

        // What the next session response says.
        void set_version(const std::string &version);
        void set_interactive(bool interactive);
        void set_grant_access(bool grant);

        // What device/client list requests return; may change while a session is open.
        void set_devices(const std::vector<FakeDevice> &devices);
        void set_clients(const std::vector<FakeClient> &clients);

        // What clients sent.
        std::vector<SessionRequest> session_requests() const;
        int device_list_requests() const;
        int client_list_requests() const;

        // The objects a session response points the client at.
        TestEvent devicesChanged;
        TestEvent clientsChanged;
        TestEvent globalStateChanged;    // hapticReadEventName
        TestEvent sessionStateChanged;   // hapticWriteEventName
        TestSharedMemory globalState;    // hapticReadSharedMemoryName
        TestSharedMemory sessionState;   // hapticWriteSharedMemoryName

    private:
        struct Client;
        static DWORD WINAPI listener_main(LPVOID self);
        static DWORD WINAPI client_main(LPVOID client);
        static HANDLE create_instance(bool first);
        static void stop_thread(HANDLE thread);
        void listen();
        void serve_client(HANDLE pipe);

        HANDLE listening_;            // the instance waiting for the next client
        HANDLE listener_;
        volatile LONG stop_ = 0;
        std::mutex threadsLock_;
        std::vector<HANDLE> clientThreads_;

        mutable std::mutex lock_;
        std::string version_ = "9.9.9-test";
        bool interactive_ = true;
        bool grantAccess_ = true;
        std::vector<FakeDevice> devices_;
        std::vector<FakeClient> clients_;
        std::vector<SessionRequest> sessionRequests_;
        int deviceListRequests_ = 0;
        int clientListRequests_ = 0;
    };
}
