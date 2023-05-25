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

PipeChannel::PipeChannel() : stop_thread(false) {

    buffer = std::vector<char>(HeaderSize);
    pOverlapped = nullptr;

    open_pipe();

    std::stringstream debugText;
    debugText << "buffer size: " << buffer.capacity();
    log(debugText.str());

    // Spin off thread for reading
    async_read_thread = std::thread([this] { this->read_thread(); } );
}

PipeChannel::~PipeChannel() {
    close_pipe();

    delete pOverlapped;

    stop_thread = true;
    async_read_thread.join();
}

void PipeChannel::open_pipe() {
    pipe = CreateFile(
        TEXT("\\\\.\\pipe\\contact-ci-service"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        std::string errorText = "Failed to open named pipe: CreateFile; GetLastError = "
                                + std::to_string(GetLastError());
        log(errorText);
        throw std::runtime_error(errorText);
    }

    DWORD pipeMode = PIPE_READMODE_BYTE;
    WINBOOL setPipeStateSuccess = SetNamedPipeHandleState(pipe, &pipeMode, NULL, NULL);

    if (!setPipeStateSuccess) {
        std::string errorText = "Failed to open named pipe: SetNamedPipeHandleState; GetLastError = "
                                + std::to_string(GetLastError());
        log(errorText);
        throw std::runtime_error(errorText);
    }
}

void PipeChannel::read_thread() {
    log("Starting async read!");
    std::string data = receive_delimited();
}

/*void PipeChannel::try_receive() {
    log("starting receive...");

    pOverlapped = new OVERLAPPED;
    ZeroMemory(pOverlapped, sizeof(OVERLAPPED));
    // hEvent unused if using completion routine, so we're free to use itx
    pOverlapped->hEvent = this;

    log("reading...");
    WINBOOL result = ReadFileEx(pipe, &buffer[0], HeaderSize, pOverlapped, async_read_completion_routine);
    if (!result) {
        std::string errorText = "Failed to read async: ReadFileEx; GetLastError = "
                                + std::to_string(GetLastError());
        log(errorText);
        throw std::runtime_error(errorText);
    }

    // We need to check this regardless of success
    DWORD lastError = GetLastError();
    if (lastError != ERROR_SUCCESS) {
        std::string errorText = "Failed to read async: ReadFileEx; GetLastError = "
                                + std::to_string(lastError);
        log(errorText);
        throw std::runtime_error(errorText);
    }
}*/

void PipeChannel::close_pipe() {
    CloseHandle(pipe);
}

void PipeChannel::send(std::string data) {
    int retryCount = 3;
    std::string errorText;

    do {
        DWORD written = 0;

        WINBOOL writeSuccess = WriteFile(pipe, data.c_str(), (DWORD) data.length(), &written, NULL);

        // TODO strip this out
        /*std::stringstream sentBytes;
        sentBytes << "Sent " << data.length() << " bytes.";
        log(sentBytes.str());

        if (data.length() > 0) {
            std::stringstream dataSent;

            dataSent << "Data: [ " << std::to_string(data.c_str()[0]);
            for (int i = 1; i < data.length(); i++) {
                dataSent << "," << std::to_string(data.c_str()[i]);
            }
            dataSent << " ]";

            log(dataSent.str());
        }*/

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