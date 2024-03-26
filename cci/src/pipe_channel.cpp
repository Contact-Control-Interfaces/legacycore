//
// Created by john_contactci on 7/27/2022.
//

#include "pipe_channel.h"

#include <stdexcept>

using namespace contactci;

PipeChannel::PipeChannel() : pipe(nullptr) {
    open_pipe();
}

PipeChannel::PipeChannel(contactci::PipeChannel &&other)
    : pipe(other.pipe) {
    other.pipe = nullptr;
}

PipeChannel::~PipeChannel() {
    close_pipe();
}

void PipeChannel::open_pipe() {
    pipe = CreateFile(
        TEXT("\\\\.\\pipe\\contact-ci-service"),
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

void PipeChannel::close_pipe() {
    if (pipe != nullptr)
        CloseHandle(pipe);
}

void PipeChannel::send(std::string data) {
    int retryCount = 3;
    std::string errorText;

    do {
        DWORD written = 0;

        BOOL writeSuccess = WriteFile(pipe, data.c_str(), (DWORD) data.length(), &written, nullptr);

        if (writeSuccess)
            return;

        errorText = "ERROR: Failed to write to named pipe: WriteFile; GetLastError = "
                                + std::to_string(GetLastError());
        close_pipe(); //TODO this may throw exception?
        open_pipe();

        retryCount--;
    } while (retryCount > 0);

    throw std::runtime_error(errorText);
}

void PipeChannel::flush() {
    BOOL flushSuccess = FlushFileBuffers(pipe);
    if (!flushSuccess) {
        std::string errorText = "ERROR: Failed to flush named pipe: FlushFileBuffers; GetLastError = "
                                + std::to_string(GetLastError());
        throw std::runtime_error(errorText);
    }
}

std::string PipeChannel::receive(uint32_t numBytes) {
    DWORD read = 0;
    std::vector<char> buffer(numBytes);
    while (read != numBytes) {
        DWORD read_this_loop = 0;
        BOOL readSuccess = ReadFile(pipe, &buffer[read], numBytes - read, &read_this_loop, nullptr);
        if (!readSuccess) {
            throw std::runtime_error(
                    std::string("Failed to read message from named pipe: ReadFile; GetLastError = ")
                    + std::to_string(GetLastError())
                    );
        }
        read += read_this_loop;
    }

    return { buffer.begin(), buffer.end() };
}
