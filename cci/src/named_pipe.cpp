//
// Created by john_contactci on 4/16/2024.
//

#include "cci/named_pipe.h"
#include "cci/error.h"

#include <stdexcept>

using namespace contactci;

NamedPipe::NamedPipe(contactci::NamedPipe &&other) noexcept : pipe(other.pipe) {
    other.pipe = nullptr;
}

NamedPipe::NamedPipe(const std::string &name) {
    pipe = CreateFile(
        name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (pipe == INVALID_HANDLE_VALUE)
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_OPEN);

    DWORD pipeMode = PIPE_READMODE_BYTE;
    BOOL setPipeStateSuccess = SetNamedPipeHandleState(pipe, &pipeMode, nullptr, nullptr);

    if (!setPipeStateSuccess)
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_OPEN);
}

NamedPipe::~NamedPipe() {
    CloseHandle(pipe);
}

HANDLE NamedPipe::get_handle() const {
    return pipe;
}