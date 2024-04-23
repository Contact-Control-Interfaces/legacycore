//
// Created by john_contactci on 7/27/2022.
//

#include "pipe_channel.h"

#include <stdexcept>

using namespace contactci;

static const std::string PIPE_NAME = R"(\\.\pipe\contact-ci-service)";

PipeChannel::PipeChannel() : pipe(std::make_unique<NamedPipe>(PIPE_NAME)) {}
PipeChannel::PipeChannel(PipeChannel &&other) : pipe(std::move(other.pipe)) {}

bool PipeChannel::is_connected() {
    return WriteFile(pipe->get_handle(), nullptr, 0, nullptr, nullptr);
}

void PipeChannel::send(std::string data) {
    int retryCount = 3;
    std::string errorText;

    do {
        DWORD written = 0;

        BOOL writeSuccess = WriteFile(pipe->get_handle(), data.c_str(), (DWORD) data.length(), &written, nullptr);

        if (writeSuccess)
            return;

        errorText = "ERROR: Failed to write to named pipe: WriteFile; GetLastError = "
                    + std::to_string(GetLastError());

        // Reopen pipe
        pipe = std::make_unique<NamedPipe>(PIPE_NAME);

        retryCount--;
    } while (retryCount > 0);

    pipe.reset();
    throw std::runtime_error(errorText);
}

void PipeChannel::flush() {
    BOOL flushSuccess = FlushFileBuffers(pipe->get_handle());
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
        BOOL readSuccess = ReadFile(pipe->get_handle(), &buffer[read], numBytes - read, &read_this_loop, nullptr);
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
