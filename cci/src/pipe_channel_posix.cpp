//
// Created by zach_contactci on 10/13/2026.
//

#include "cci/pipe_channel.h"
#include "cci/error.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <sys/socket.h>

using namespace contactci;

// From communication/docs/linux-core-service-contract.md §1
static const std::string PIPE_NAME = "/run/contactci/service.sock";

// CONTACTCI_SERVICE_SOCKET, when set and not empty, replaces the compiled-in path. It exists for the loopback tests
// (test/unit/support/fake_daemon_posix.*), which can't create /run/contactci on a CI runner, and for running a
// Service from a build tree. secure_getenv ignores it in a setuid or setgid process, so it can't redirect a
// privileged Client.
static std::string service_socket_path() {
    const char *override_path = secure_getenv("CONTACTCI_SERVICE_SOCKET");

    return override_path != nullptr && *override_path != '\0' ? std::string(override_path) : PIPE_NAME;
}

PipeChannel::PipeChannel() : pipe(std::make_unique<NamedPipe>(service_socket_path())) {}
PipeChannel::PipeChannel(PipeChannel &&other)
    : pipe(std::move(other.pipe)),
      sessionDescriptors(std::move(other.sessionDescriptors)),
      sessionDescriptorsTruncated(other.sessionDescriptorsTruncated),
      sessionDescriptorsTaken(other.sessionDescriptorsTaken) {}

bool PipeChannel::is_connected() {
    if (!pipe)
        return false;

    // Look for a hang-up without reading, so this can't race a monitor thread's request on the same socket
    pollfd poll_fd { pipe->native_handle(), POLLRDHUP, 0 };

    if (poll(&poll_fd, 1, 0) < 0)
        return false;

    return !(poll_fd.revents & (POLLRDHUP | POLLHUP | POLLERR | POLLNVAL));
}

void PipeChannel::send(std::string data) {
    if (!pipe)
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_WRITE);

    // Unlike Windows, a failed write doesn't reopen the socket: one connection is one Session, and a new
    // connection would carry no session init. Nor does it release the socket: `pipe` never changes after
    // construction, so is_connected() can read it from another thread without a lock. A dead socket keeps failing
    // send() and receive() and reports a hang-up to is_connected().
    std::size_t written = 0;

    while (written != data.length()) {
        // MSG_NOSIGNAL: writing to a socket the Service closed would otherwise raise SIGPIPE and kill the Client
        ssize_t written_this_loop = ::send(pipe->native_handle(), data.data() + written, data.length() - written, MSG_NOSIGNAL);

        if (written_this_loop < 0) {
            if (errno == EINTR)
                continue;

            throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_WRITE);
        }

        written += written_this_loop;
    }
}

void PipeChannel::flush() {
    // Stream sockets have no user-space buffer to flush: send() has already handed every byte to the kernel
}

// Room for exactly the most the contract sends: a reply with fewer fails the count in take_session_descriptors(),
// and one with more is truncated by the kernel and fails on MSG_CTRUNC
static constexpr std::size_t MAX_DESCRIPTORS = 2;

std::string PipeChannel::receive(uint32_t numBytes) {
    if (!pipe)
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_READ);

    std::uint32_t read = 0;
    std::vector<char> buffer(numBytes);

    while (read != numBytes) {
        // recvmsg with a control buffer on every call, not just the first: the Service attaches the Session's
        // descriptors to the first byte of the session init reply, and a short read can split that frame anywhere
        // (contract §3)
        iovec data { &buffer[read], numBytes - read };
        alignas(cmsghdr) char control[CMSG_SPACE(sizeof(int) * MAX_DESCRIPTORS)];

        msghdr message {};
        message.msg_iov = &data;
        message.msg_iovlen = 1;
        message.msg_control = control;
        message.msg_controllen = sizeof(control);

        // MSG_CMSG_CLOEXEC, for the same reason as SOCK_CLOEXEC on the socket: a child the Client spawns must not
        // inherit this Session's objects
        ssize_t read_this_loop = recvmsg(pipe->native_handle(), &message, MSG_CMSG_CLOEXEC);

        if (read_this_loop < 0 && errno == EINTR)
            continue;

        // Before any throw: once received, a descriptor is open in this process until something closes it
        if (read_this_loop > 0)
            keep_descriptors(message);

        // 0 means the Service closed the socket mid-message
        if (read_this_loop <= 0)
            throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_READ);

        read += read_this_loop;
    }

    return { buffer.begin(), buffer.end() };
}

void PipeChannel::keep_descriptors(msghdr &message) {
    // The kernel closes descriptors that don't fit the control buffer, so the only trace of them is this flag
    if (message.msg_flags & MSG_CTRUNC && !sessionDescriptorsTaken)
        sessionDescriptorsTruncated = true;

    for (cmsghdr *header = CMSG_FIRSTHDR(&message); header != nullptr; header = CMSG_NXTHDR(&message, header)) {
        if (header->cmsg_level != SOL_SOCKET || header->cmsg_type != SCM_RIGHTS)
            continue;

        std::size_t count = (header->cmsg_len - CMSG_LEN(0)) / sizeof(int);

        for (std::size_t i = 0; i < count; i++) {
            int fd;
            std::memcpy(&fd, CMSG_DATA(header) + i * sizeof(int), sizeof(int));
            UniqueFd descriptor(fd);

            // Only the session init reply may carry descriptors. One on a later frame is closed here, by `descriptor`
            if (!sessionDescriptorsTaken)
                sessionDescriptors.push_back(std::move(descriptor));
        }
    }
}

std::vector<UniqueFd> PipeChannel::take_session_descriptors(std::size_t expected) {
    // Only the constructing Session calls this, before its monitor threads start, so it needs no lock
    std::vector<UniqueFd> descriptors = std::move(sessionDescriptors);
    sessionDescriptors.clear();
    sessionDescriptorsTaken = true;

    if (sessionDescriptorsTruncated || descriptors.size() != expected)
        throw contactci::Exception(CCI_ERR_SHM_FAILED_TO_OPEN);

    return descriptors;
}
