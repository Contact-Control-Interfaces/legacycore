//
// Created by zach_contactci on 10/13/2026.
//

#include "cci/named_pipe.h"
#include "cci/error.h"

#include <unistd.h>

using namespace contactci;

NamedPipe::NamedPipe(contactci::NamedPipe &&other) noexcept : pipe(other.pipe) {
    other.pipe = -1;
}

NamedPipe::NamedPipe(const std::string &name) : pipe(-1) {
    // TODO(DEV-6): connect a SOCK_STREAM Unix socket to `name`
    throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_OPEN);
}

NamedPipe::~NamedPipe() {
    if (pipe >= 0)
        close(pipe);
}

NativeHandle NamedPipe::native_handle() const {
    return pipe;
}
