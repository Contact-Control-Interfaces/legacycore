#include "fake_service.h"

#include <stdexcept>

#include "common.pb.h"
#include "info.pb.h"

namespace testsupport {
    namespace {
        // The header is two fixed32 fields, so its encoded size never changes.
        std::size_t header_size() {
            PacketHeader header;
            header.set_opcode(1);
            header.set_length(1);
            return header.ByteSizeLong();
        }

        bool read_exact(HANDLE pipe, void *buffer, DWORD size) {
            auto *out = static_cast<char *>(buffer);
            DWORD total = 0;
            while (total < size) {
                DWORD read = 0;
                if (!ReadFile(pipe, out + total, size - total, &read, nullptr))
                    return false;     // client gone, or cancelled by the destructor
                total += read;        // 0 is legal: Session::is_connected() writes zero bytes
            }
            return true;
        }

        void respond(HANDLE pipe, int opcode, const std::string &body) {
            PacketHeader header;
            header.set_opcode(opcode);
            header.set_length(static_cast<uint32_t>(body.size()));
            std::string data = header.SerializeAsString() + body;
            DWORD written = 0;
            WriteFile(pipe, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
        }
    }

    struct FakeService::Client {
        FakeService *service;
        HANDLE pipe;
    };

    bool FakeService::real_service_running() {
        if (WaitNamedPipeA(PipeName, 1))
            return true;
        return GetLastError() != ERROR_FILE_NOT_FOUND;
    }

    HANDLE FakeService::create_instance(bool first) {
        // FIRST_PIPE_INSTANCE on the first one: fail visibly if anything else owns the name.
        DWORD openMode = PIPE_ACCESS_DUPLEX | (first ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0);
        return CreateNamedPipeA(PipeName, openMode, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                PIPE_UNLIMITED_INSTANCES, 64 * 1024, 64 * 1024, 0, nullptr);
    }

    // The thread is blocked in ConnectNamedPipe or ReadFile; cancel until it exits.
    void FakeService::stop_thread(HANDLE thread) {
        while (WaitForSingleObject(thread, 10) == WAIT_TIMEOUT)
            CancelSynchronousIo(thread);
        CloseHandle(thread);
    }

    FakeService::FakeService() {
        listening_ = create_instance(true);
        if (listening_ == INVALID_HANDLE_VALUE)
            throw std::runtime_error("could not create " + std::string(PipeName) +
                                     " (error " + std::to_string(GetLastError()) + ")");
        listener_ = CreateThread(nullptr, 0, &FakeService::listener_main, this, 0, nullptr);
        if (listener_ == nullptr)
            throw std::runtime_error("could not start the fake service thread");
    }

    FakeService::~FakeService() {
        InterlockedExchange(&stop_, 1);
        stop_thread(listener_);
        std::lock_guard g(threadsLock_);
        for (HANDLE thread : clientThreads_)
            stop_thread(thread);
        CloseHandle(listening_);
    }

    void FakeService::set_version(const std::string &version) { std::lock_guard g(lock_); version_ = version; }
    void FakeService::set_interactive(bool interactive) { std::lock_guard g(lock_); interactive_ = interactive; }
    void FakeService::set_grant_access(bool grant) { std::lock_guard g(lock_); grantAccess_ = grant; }
    void FakeService::set_devices(const std::vector<FakeDevice> &devices) { std::lock_guard g(lock_); devices_ = devices; }
    void FakeService::set_clients(const std::vector<FakeClient> &clients) { std::lock_guard g(lock_); clients_ = clients; }

    std::vector<SessionRequest> FakeService::session_requests() const { std::lock_guard g(lock_); return sessionRequests_; }
    int FakeService::device_list_requests() const { std::lock_guard g(lock_); return deviceListRequests_; }
    int FakeService::client_list_requests() const { std::lock_guard g(lock_); return clientListRequests_; }

    DWORD WINAPI FakeService::listener_main(LPVOID self) {
        static_cast<FakeService *>(self)->listen();
        return 0;
    }

    DWORD WINAPI FakeService::client_main(LPVOID param) {
        auto *client = static_cast<Client *>(param);
        client->service->serve_client(client->pipe);
        DisconnectNamedPipe(client->pipe);
        CloseHandle(client->pipe);
        delete client;
        return 0;
    }

    void FakeService::listen() {
        while (!InterlockedCompareExchange(&stop_, 0, 0)) {
            BOOL connected = ConnectNamedPipe(listening_, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED;
            if (!connected)
                continue;   // cancelled: the loop condition decides

            // Hand this client its own thread, and put a fresh instance up for the next one.
            auto *client = new Client{this, listening_};
            listening_ = create_instance(false);
            HANDLE thread = CreateThread(nullptr, 0, &FakeService::client_main, client, 0, nullptr);
            std::lock_guard g(threadsLock_);
            clientThreads_.push_back(thread);
        }
    }

    void FakeService::serve_client(HANDLE pipe) {
        std::string headerBytes(header_size(), '\0');
        while (read_exact(pipe, headerBytes.data(), static_cast<DWORD>(headerBytes.size()))) {
            PacketHeader header;
            header.ParseFromString(headerBytes);
            std::string body(header.length(), '\0');
            if (header.length() > 0 && !read_exact(pipe, body.data(), header.length()))
                return;

            std::lock_guard g(lock_);
            switch (header.opcode()) {
                case OpCode::opSessionInitializationRequestMessage: {
                    SessionInitializationRequestMessage request;
                    request.ParseFromString(body);
                    sessionRequests_.push_back({request.ishapticsession(), request.wantshapticwriteaccess()});

                    SessionInitializationResponseMessage response;
                    response.set_version(version_);
                    response.set_isinteractive(interactive_);
                    response.set_ishapticaccessgranted(grantAccess_);
                    response.set_hapticreadsharedmemoryname(globalState.name());
                    response.set_hapticreadeventname(globalStateChanged.name());
                    response.set_hapticwritesharedmemoryname(sessionState.name());
                    response.set_hapticwriteeventname(sessionStateChanged.name());
                    response.set_deviceschangedeventname(devicesChanged.name());
                    response.set_clientschangedeventname(clientsChanged.name());
                    respond(pipe, OpCode::opSessionInitializationResponseMessage, response.SerializeAsString());
                    break;
                }
                case OpCode::opDeviceListRequestMessage: {
                    deviceListRequests_++;
                    DeviceListResponseMessage response;
                    for (const auto &d : devices_) {
                        auto *m = response.add_devices();
                        m->set_productline(d.productLine);
                        m->set_serialnumber(d.serialNumber);
                        m->set_isright(d.isRight);
                        m->set_isconnected(d.isConnected);
                    }
                    respond(pipe, OpCode::opDeviceListResponseMessage, response.SerializeAsString());
                    break;
                }
                case OpCode::opClientListRequestMessage: {
                    clientListRequests_++;
                    ClientListResponseMessage response;
                    for (const auto &c : clients_) {
                        auto *m = response.add_clients();
                        m->set_processid(c.processId);
                        m->set_processname(c.processName);
                    }
                    respond(pipe, OpCode::opClientListResponseMessage, response.SerializeAsString());
                    break;
                }
                default:
                    break;  // libcci sends nothing else
            }
        }
    }
}
