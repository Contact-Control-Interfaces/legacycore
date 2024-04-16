//
// Created by john_contactci on 4/16/2024.
//

#include "named_pipe.h"

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

    if (pipe == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(
            std::string("Failed to open named pipe: CreateFile; GetLastError = ")
            + std::to_string(GetLastError())
        );
    }

    DWORD pipeMode = PIPE_READMODE_BYTE;
    BOOL setPipeStateSuccess = SetNamedPipeHandleState(pipe, &pipeMode, nullptr, nullptr);

    if (!setPipeStateSuccess) {
        throw std::runtime_error(
            std::string("Failed to open named pipe: SetNamedPipeHandleState; GetLastError = ")
            + std::to_string(GetLastError())
        );
    }
}

NamedPipe::~NamedPipe() {
    CloseHandle(pipe);
}

HANDLE NamedPipe::get_handle() const {
    return pipe;
}