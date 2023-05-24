//
// Created by john_contactci on 7/27/2022.
//

#pragma once

#include <cci_lib_defs.h>
#include <io/channel.h>

#include <windows.h>
#include <vector>

namespace contactci::io {

    class CCI_API_CLASS(PipeChannel) : public Channel {
    public:
        PipeChannel();
        ~PipeChannel();

        void send(std::string data) override;
        std::string receive(uint32_t numBytes) override;
        void flush() override;

        void try_receive();

    protected:
        void open_pipe();
        void close_pipe();

        void read_completion_routine(unsigned long dwErrorCode, unsigned long dwNumberOfBytesTransferred, _OVERLAPPED* lpOverlapped);

    private:
        HANDLE pipe;
        std::vector<char> buffer;
        OVERLAPPED* pOverlapped;
    };

    void another_read_completion_routine(unsigned long dwErrorCode, unsigned long dwNumberOfBytesTransferred, _OVERLAPPED* lpOverlapped){
        auto channel = reinterpret_cast<PipeChannel*>(lpOverlapped->hEvent);
        std::cout << "got " << dwNumberOfBytesTransferred << " bytes!" << std::endl;

        std::cout << "restarting read..." << std::endl;
        channel->try_receive();
    }

    // basically just a static class
    class CCI_API_CLASS(StdOutChannel) : public Channel {
    public:
        void send(std::string data) override;
        std::string receive(uint32_t numBytes) override;
        void flush() override;
    };
}