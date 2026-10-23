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

        // Contract §3: sendmsg with SCM_RIGHTS on the call that sends the first byte of the frame.
        ssize_t send_with_descriptors(int connection, const char *data, std::size_t size,
                                      const std::vector<int> &descriptors) {
            iovec payload{const_cast<char *>(data), size};
            std::vector<char> control(CMSG_SPACE(sizeof(int) * descriptors.size()));
            msghdr message{};
            message.msg_iov = &payload;
            message.msg_iovlen = 1;
            message.msg_control = control.data();
            message.msg_controllen = control.size();
            cmsghdr *header = CMSG_FIRSTHDR(&message);
            header->cmsg_level = SOL_SOCKET;
            header->cmsg_type = SCM_RIGHTS;
            header->cmsg_len = CMSG_LEN(sizeof(int) * descriptors.size());
            std::memcpy(CMSG_DATA(header), descriptors.data(), sizeof(int) * descriptors.size());
            return sendmsg(connection, &message, MSG_NOSIGNAL);
        }

        // `split` sends the first byte on its own. Descriptors go with it and never again, as §3 requires.
        void respond(int connection, int opcode, const std::string &body,
                     const std::vector<int> &descriptors = {}, bool split = false) {
            PacketHeader header;
            header.set_opcode(opcode);
            header.set_length(static_cast<uint32_t>(body.size()));
            std::string data = header.SerializeAsString() + body;
            std::size_t sent = 0;
            while (sent < data.size()) {
                std::size_t size = split && sent == 0 ? 1 : data.size() - sent;
                ssize_t now = sent == 0 && !descriptors.empty()
                                  ? send_with_descriptors(connection, data.data(), size, descriptors)
                                  : send(connection, data.data() + sent, size, MSG_NOSIGNAL);
                if (now < 0 && errno == EINTR)
                    continue;
                if (now <= 0)
                    return;   // client gone; its next read fails, which is what a test wants to see
                sent += static_cast<std::size_t>(now);
            }
        }

        // Descriptors the daemon creates for one response only, closed once it is sent.
        struct OneOffDescriptors {
            std::vector<int> fds;
            int add(std::size_t size) { fds.push_back(create_sealed_object(size)); return fds.back(); }
            ~OneOffDescriptors() { for (int fd : fds) close(fd); }
        };

        // Contract §3: any payload whose first field claims more bytes than follow.
        const std::string UnparseablePayload("\x0a\xff", 2);
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
    void FakeDaemon::set_protocol_version(uint32_t version) { std::lock_guard g(lock_); protocolVersion_ = version; }
    void FakeDaemon::set_session_reply_opcode(int opcode) { std::lock_guard g(lock_); sessionReplyOpcode_ = opcode; }
    void FakeDaemon::set_unparseable_session_reply(bool unparseable) { std::lock_guard g(lock_); unparseableSessionReply_ = unparseable; }
    void FakeDaemon::set_session_attachments(SessionAttachments attachments) { std::lock_guard g(lock_); sessionAttachments_ = attachments; }
    void FakeDaemon::set_split_session_reply(bool split) { std::lock_guard g(lock_); splitSessionReply_ = split; }
    void FakeDaemon::set_descriptor_on_device_lists(bool attach) { std::lock_guard g(lock_); descriptorOnDeviceLists_ = attach; }
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
            OneOffDescriptors oneOff;
            std::vector<int> descriptors;
            bool split = false;
            bool closeAfterResponse = false;
            switch (header.opcode()) {
                case OpCode::opSessionInitializationRequestMessage: {
                    SessionInitializationRequestMessage request;
                    if (!request.ParseFromString(body))
                        protocolErrors_.push_back("a session init request that does not parse");
                    sessionRequests_.push_back({request.ishapticsession(), request.wantshapticwriteaccess(),
                                                request.protocolversion()});
                    if (request.protocolversion() != CONTACTCI_PROTOCOL_VERSION)
                        protocolErrors_.push_back("a session init request for protocol version " +
                                                  std::to_string(request.protocolversion()) + ", not " +
                                                  std::to_string(CONTACTCI_PROTOCOL_VERSION));

                    // §3: a reply with any other version than the one requested is a refusal, and the Service
                    // closes the connection after sending it, having created nothing.
                    bool refusing = protocolVersion_ != request.protocolversion();
                    bool writes = !refusing && grantAccess_ && request.wantshapticwriteaccess();

                    SessionInitializationResponseMessage message;
                    message.set_version(version_);
                    message.set_isinteractive(interactive_);
                    message.set_ishapticaccessgranted(grantAccess_);
                    message.set_protocolversion(protocolVersion_);
                    message.set_hapticreadsharedmemoryname(globalState.name());
                    message.set_hapticreadeventname(globalStateChanged.name());
                    if (writes) {
                        message.set_hapticwritesharedmemoryname("fd:0");
                        message.set_hapticwriteeventname("fd:1");
                    }
                    message.set_deviceschangedeventname(devicesChanged.name());
                    message.set_clientschangedeventname(clientsChanged.name());
                    opcode = sessionReplyOpcode_;
                    response = unparseableSessionReply_ ? UnparseablePayload : message.SerializeAsString();

                    int region = sessionState.fd();
                    int counter = sessionStateChanged.fd();
                    switch (sessionAttachments_) {
                        case SessionAttachments::AsContract:
                            if (writes) descriptors = {region, counter};
                            break;
                        case SessionAttachments::Nothing:
                            break;
                        case SessionAttachments::RegionOnly:
                            if (writes) descriptors = {region};
                            break;
                        case SessionAttachments::OneTooMany:
                            if (writes) descriptors = {region, counter, oneOff.add(CONTACTCI_CHANGE_COUNTER_SIZE)};
                            break;
                        case SessionAttachments::ShortRegion:
                            if (writes) descriptors = {oneOff.add(CONTACTCI_HAPTIC_STATE_SIZE), counter};
                            break;
                        case SessionAttachments::LongCounter:
                            if (writes) descriptors = {region, oneOff.add(2 * CONTACTCI_CHANGE_COUNTER_SIZE)};
                            break;
                        case SessionAttachments::EvenWithoutWriteAccess:
                            descriptors = {region, counter};
                            break;
                    }
                    split = splitSessionReply_;
                    closeAfterResponse = refusing;
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
                    if (descriptorOnDeviceLists_)
                        descriptors = {oneOff.add(CONTACTCI_CHANGE_COUNTER_SIZE)};
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
            respond(connection, opcode, response, descriptors, split);
            if (closeAfterResponse)
                return;   // the client thread closes the connection
        }
    }
}
