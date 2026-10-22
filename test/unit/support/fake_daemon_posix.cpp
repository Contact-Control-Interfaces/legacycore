#include "fake_daemon_posix.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "common.pb.h"
#include "info.pb.h"

namespace testsupport {
    namespace {
        // Contract §2: two required fixed32 fields, always exactly 10 bytes.
        constexpr std::size_t HeaderSize = 10;

        std::runtime_error os_error(const std::string &what) {
            return std::runtime_error("fake daemon: " + what + ": " + std::strerror(errno));
        }

        // False when the client closed the connection (or disconnect_all() shut it down).
        bool read_exact(int connection, char *buffer, std::size_t size) {
            std::size_t total = 0;
            while (total < size) {
                ssize_t read = recv(connection, buffer + total, size - total, 0);
                if (read < 0 && errno == EINTR)
                    continue;
                if (read <= 0)
                    return false;
                total += static_cast<std::size_t>(read);
            }
            return true;
        }

        void respond(int connection, int opcode, const std::string &body) {
            PacketHeader header;
            header.set_opcode(opcode);
            header.set_length(static_cast<uint32_t>(body.size()));
            std::string data = header.SerializeAsString() + body;
            std::size_t sent = 0;
            while (sent < data.size()) {
                ssize_t now = send(connection, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
                if (now < 0 && errno == EINTR)
                    continue;
                if (now <= 0)
                    return;   // client gone; its next read fails, which is what a test wants to see
                sent += static_cast<std::size_t>(now);
            }
        }
    }

    std::string missing_socket_path() {
        return "/tmp/cci-no-daemon-" + std::to_string(getpid()) + "/service.sock";
    }

    ScopedEnvironment::ScopedEnvironment(const char *name, const std::string &value) : name_(name) {
        const char *old = std::getenv(name);
        hadValue_ = old != nullptr;
        if (hadValue_)
            oldValue_ = old;
        setenv(name, value.c_str(), 1);
    }

    ScopedEnvironment::~ScopedEnvironment() {
        if (hadValue_)
            setenv(name_.c_str(), oldValue_.c_str(), 1);
        else
            unsetenv(name_.c_str());
    }

    FakeDaemon::FakeDaemon() {
        char directory[] = "/tmp/cci-daemon-XXXXXX";   // short: sun_path holds 108 bytes
        if (mkdtemp(directory) == nullptr)
            throw os_error("mkdtemp");
        directory_ = directory;
        socketPath_ = directory_ + "/service.sock";

        listener_ = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (listener_ < 0)
            throw os_error("socket");
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        std::memcpy(address.sun_path, socketPath_.c_str(), socketPath_.size() + 1);
        if (bind(listener_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0)
            throw os_error("bind " + socketPath_);
        if (listen(listener_, 16) < 0)
            throw os_error("listen");
        if (pipe2(wake_, O_CLOEXEC) < 0)
            throw os_error("pipe2");

        pointCoreHere_ = std::make_unique<ScopedEnvironment>(ServiceSocketVariable, socketPath_);
        acceptThread_ = std::thread(&FakeDaemon::accept_loop, this);
    }

    FakeDaemon::~FakeDaemon() {
        {
            std::lock_guard g(connectionsLock_);
            stopping_ = true;
        }
        char stop = 1;
        (void) !write(wake_[1], &stop, 1);
        acceptThread_.join();

        // accept_loop has stopped, so clientThreads_ no longer changes.
        disconnect_all();
        for (auto &thread : clientThreads_)
            thread.join();

        close(listener_);
        close(wake_[0]);
        close(wake_[1]);
        unlink(socketPath_.c_str());
        rmdir(directory_.c_str());
    }

    void FakeDaemon::set_version(const std::string &version) { std::lock_guard g(lock_); version_ = version; }
    void FakeDaemon::set_interactive(bool interactive) { std::lock_guard g(lock_); interactive_ = interactive; }
    void FakeDaemon::set_grant_access(bool grant) { std::lock_guard g(lock_); grantAccess_ = grant; }
    void FakeDaemon::set_drop_on_device_list(bool drop) { std::lock_guard g(lock_); dropOnDeviceList_ = drop; }
    void FakeDaemon::set_devices(const std::vector<FakeDevice> &devices) { std::lock_guard g(lock_); devices_ = devices; }
    void FakeDaemon::set_clients(const std::vector<FakeClient> &clients) { std::lock_guard g(lock_); clients_ = clients; }

    std::vector<SessionRequest> FakeDaemon::session_requests() const { std::lock_guard g(lock_); return sessionRequests_; }
    int FakeDaemon::device_list_requests() const { std::lock_guard g(lock_); return deviceListRequests_; }
    int FakeDaemon::client_list_requests() const { std::lock_guard g(lock_); return clientListRequests_; }
    std::vector<std::string> FakeDaemon::protocol_errors() const { std::lock_guard g(lock_); return protocolErrors_; }

    int FakeDaemon::connections_accepted() const { std::lock_guard g(connectionsLock_); return connectionsAccepted_; }
    int FakeDaemon::connections_open() const {
        std::lock_guard g(connectionsLock_);
        return static_cast<int>(openConnections_.size());
    }

    // A shut-down connection's recv() returns 0, so its thread sees the hang-up and
    // closes it. The fd stays open (and so is never reused) until that thread closes
    // it under connectionsLock_.
    void FakeDaemon::disconnect_all() {
        std::lock_guard g(connectionsLock_);
        for (int connection : openConnections_)
            shutdown(connection, SHUT_RDWR);
    }

    void FakeDaemon::accept_loop() {
        for (;;) {
            pollfd fds[2] = {{listener_, POLLIN, 0}, {wake_[0], POLLIN, 0}};
            if (poll(fds, 2, -1) < 0) {
                if (errno == EINTR)
                    continue;
                return;
            }
            if (fds[1].revents != 0)
                return;
            if ((fds[0].revents & POLLIN) == 0)
                continue;

            int connection = accept4(listener_, nullptr, nullptr, SOCK_CLOEXEC);
            if (connection < 0)
                continue;

            std::lock_guard g(connectionsLock_);
            if (stopping_) {
                close(connection);
                return;
            }
            connectionsAccepted_++;
            openConnections_.insert(connection);
            clientThreads_.emplace_back([this, connection] {
                serve(connection);
                std::lock_guard closing(connectionsLock_);
                openConnections_.erase(connection);
                close(connection);
            });
        }
    }

    void FakeDaemon::serve(int connection) {
        bool first = true;
        std::string headerBytes(HeaderSize, '\0');
        while (read_exact(connection, headerBytes.data(), HeaderSize)) {
            PacketHeader header;
            if (!header.ParseFromString(headerBytes)) {
                std::lock_guard g(lock_);
                protocolErrors_.push_back("a 10-byte header that does not parse as a PacketHeader");
                return;
            }
            std::string body(header.length(), '\0');
            if (header.length() > 0 && !read_exact(connection, body.data(), header.length()))
                return;

            std::unique_lock g(lock_);
            bool isSessionInit = header.opcode() == OpCode::opSessionInitializationRequestMessage;
            if (first != isSessionInit)
                protocolErrors_.push_back(first ? "the first message on a connection was opcode " +
                                                  std::to_string(header.opcode()) + ", not 44 (session init)"
                                                : "a second session init on the same connection");
            first = false;

            int opcode = 0;
            std::string response;
            switch (header.opcode()) {
                case OpCode::opSessionInitializationRequestMessage: {
                    SessionInitializationRequestMessage request;
                    if (!request.ParseFromString(body))
                        protocolErrors_.push_back("a session init request that does not parse");
                    sessionRequests_.push_back({request.ishapticsession(), request.wantshapticwriteaccess()});

                    SessionInitializationResponseMessage message;
                    message.set_version(version_);
                    message.set_isinteractive(interactive_);
                    message.set_ishapticaccessgranted(grantAccess_);
                    message.set_hapticreadsharedmemoryname(globalState.name());
                    message.set_hapticreadeventname(globalStateChanged.name());
                    message.set_hapticwritesharedmemoryname(sessionState.name());
                    message.set_hapticwriteeventname(sessionStateChanged.name());
                    message.set_deviceschangedeventname(devicesChanged.name());
                    message.set_clientschangedeventname(clientsChanged.name());
                    opcode = OpCode::opSessionInitializationResponseMessage;
                    response = message.SerializeAsString();
                    break;
                }
                case OpCode::opDeviceListRequestMessage: {
                    deviceListRequests_++;
                    if (dropOnDeviceList_)
                        return;   // the client thread closes the connection
                    DeviceListResponseMessage message;
                    for (const auto &d : devices_) {
                        auto *m = message.add_devices();
                        m->set_productline(d.productLine);
                        m->set_serialnumber(d.serialNumber);
                        m->set_isright(d.isRight);
                        m->set_isconnected(d.isConnected);
                    }
                    opcode = OpCode::opDeviceListResponseMessage;
                    response = message.SerializeAsString();
                    break;
                }
                case OpCode::opClientListRequestMessage: {
                    clientListRequests_++;
                    ClientListResponseMessage message;
                    for (const auto &c : clients_) {
                        auto *m = message.add_clients();
                        m->set_processid(c.processId);
                        m->set_processname(c.processName);
                    }
                    opcode = OpCode::opClientListResponseMessage;
                    response = message.SerializeAsString();
                    break;
                }
                default:
                    protocolErrors_.push_back("opcode " + std::to_string(header.opcode()) + ", which Core never sends");
                    continue;
            }
            g.unlock();
            respond(connection, opcode, response);
        }
    }
}
