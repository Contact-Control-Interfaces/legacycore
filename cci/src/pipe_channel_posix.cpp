//
// Created by zach_contactci on 10/13/2026.
//

#include "cci/pipe_channel.h"
#include "cci/error.h"

using namespace contactci;

// TODO(DEV-6): take the socket path from the constant in the `communication` submodule once it exists
static const std::string PIPE_NAME = "/run/contactci/service.sock";

PipeChannel::PipeChannel() : pipe(std::make_unique<NamedPipe>(PIPE_NAME)) {}
PipeChannel::PipeChannel(PipeChannel &&other) : pipe(std::move(other.pipe)) {}

bool PipeChannel::is_connected() {
    // TODO(DEV-6)
    return false;
}

void PipeChannel::send(std::string data) {
    // TODO(DEV-6)
    throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_WRITE);
}

void PipeChannel::flush() {
    // TODO(DEV-6)
    throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_FLUSH);
}

std::string PipeChannel::receive(uint32_t numBytes) {
    // TODO(DEV-6)
    throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_READ);
}
