//
// Created by zach_contactci on 10/13/2026.
//

#include "cci/pipe_channel.h"
#include "cci/error.h"

#include <cerrno>
#include <cstdlib>
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
PipeChannel::PipeChannel(PipeChannel &&other) : pipe(std::move(other.pipe)) {}

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

std::string PipeChannel::receive(uint32_t numBytes) {
    if (!pipe)
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_READ);

    std::uint32_t read = 0;
    std::vector<char> buffer(numBytes);

    while (read != numBytes) {
        ssize_t read_this_loop = recv(pipe->native_handle(), &buffer[read], numBytes - read, 0);

        if (read_this_loop < 0 && errno == EINTR)
            continue;

        // 0 means the Service closed the socket mid-message
        if (read_this_loop <= 0)
            throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_READ);

        read += read_this_loop;
    }

    return { buffer.begin(), buffer.end() };
}
