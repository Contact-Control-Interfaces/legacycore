//
// Created by john_contactci on 7/27/2022.
//

#include "channel.h"

#include <string>
#include <stdexcept>
#include <iostream>
#include <sstream>

#include <windows.h>

using namespace contactci::io;

PipeChannel::PipeChannel() {
    open_pipe();
}

PipeChannel::~PipeChannel() {
    close_pipe();
}

void PipeChannel::open_pipe() {
    pipe = CreateFile(
        TEXT("\\\\.\\pipe\\contact-ci-service"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(
                std::string("Failed to open named pipe: CreateFile; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }

    DWORD pipeMode = PIPE_READMODE_BYTE;
    WINBOOL setPipeStateSuccess = SetNamedPipeHandleState(pipe, &pipeMode, NULL, NULL);

    if (!setPipeStateSuccess) {
        throw std::runtime_error(
                std::string("Failed to open named pipe: SetNamedPipeHandleState; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }
}

void PipeChannel::close_pipe() {
    CloseHandle(pipe);
}

void PipeChannel::send(std::string data) {
    int retryCount = 3;
    std::string errorText;

    do {
        DWORD written = 0;

        WINBOOL writeSuccess = WriteFile(pipe, data.c_str(), (DWORD) data.length(), &written, NULL);

        if (writeSuccess)
            return;

        errorText = "ERROR: Failed to write to named pipe: WriteFile; GetLastError = "
                                + std::to_string(GetLastError());
        log(errorText);

        close_pipe(); //TODO this may throw exception?
        open_pipe();

        retryCount--;
    } while (retryCount > 0);

    throw std::runtime_error(errorText);
}

void PipeChannel::flush() {
    WINBOOL flushSuccess = FlushFileBuffers(pipe);
    if (!flushSuccess) {
        std::string errorText = "ERROR: Failed to flush named pipe: FlushFileBuffers; GetLastError = "
                                + std::to_string(GetLastError());
        log(errorText);
        throw std::runtime_error(errorText);
    }
}

std::string PipeChannel::receive(uint32_t numBytes) {
    DWORD read = 0;
    std::vector<char> buffer(numBytes);
    while (read != numBytes) {
        DWORD read_this_loop = 0;
        WINBOOL readSuccess = ReadFile(pipe, &buffer[read], numBytes - read, &read_this_loop, NULL);
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

void StdOutChannel::send(std::string data) {
    std::cout << data;
}

void StdOutChannel::flush() {
    std::cout.flush();
}

std::string StdOutChannel::receive(uint32_t numBytes) {
    std::vector<char> buffer(numBytes);
    std::cin.get(&buffer[0], numBytes);
    return {buffer.begin(), buffer.end()};
}
