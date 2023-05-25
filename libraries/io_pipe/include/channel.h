//
// Created by john_contactci on 7/27/2022.
//

#pragma once

#include <cci_lib_defs.h>
#include <io/channel.h>

#include <windows.h>
#include <vector>
#include <sstream>

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

    private:
        HANDLE pipe;
        std::vector<char> buffer;
        OVERLAPPED* pOverlapped;
    };

    // TODO find a better place to put this
    void async_read_completion_routine(unsigned long dwErrorCode, unsigned long dwNumberOfBytesTransferred, _OVERLAPPED* lpOverlapped){
        std::cout << "trying cast...";
        auto channel = reinterpret_cast<PipeChannel*>(lpOverlapped->hEvent);
        std::cout << "done" << std::endl;

        std::stringstream bytesText;
        bytesText << "got " << dwNumberOfBytesTransferred << " bytes!";
        channel->log(bytesText.str());

        channel->log("restarting read...");
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