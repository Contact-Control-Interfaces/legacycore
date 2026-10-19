//
// Created by zach_contactci on 10/13/2026.
//

#include "cci/named_pipe.h"
#include "cci/error.h"

#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace contactci;

NamedPipe::NamedPipe(contactci::NamedPipe &&other) noexcept : pipe(other.pipe) {
    other.pipe = -1;
}

NamedPipe::NamedPipe(const std::string &name) : pipe(-1) {
    sockaddr_un address {};
    address.sun_family = AF_UNIX;

    // sun_path is a fixed 108-byte buffer and must hold the terminating null
    if (name.size() >= sizeof(address.sun_path))
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_OPEN);

    std::memcpy(address.sun_path, name.c_str(), name.size() + 1);

    // CLOEXEC so a child the Client spawns can't inherit the connection and keep the Session alive after the Client exits
    pipe = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

    if (pipe < 0)
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_OPEN);

    if (connect(pipe, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        // The destructor doesn't run when the constructor throws
        close(pipe);
        pipe = -1;
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_OPEN);
    }
}

NamedPipe::~NamedPipe() {
    if (pipe >= 0)
        close(pipe);
}

NativeHandle NamedPipe::native_handle() const {
    return pipe;
}
