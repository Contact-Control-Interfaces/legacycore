//
// Created by john_contactci on 7/27/2022.
//

#pragma once

#include <cci_lib_defs.h>
#include <io/channel.h>

#include <windows.h>
#include <vector>
#include <sstream>
#include <thread>

namespace contactci::io {

    class CCI_API_CLASS(PipeChannel) : public Channel {
    public:
        PipeChannel();
        ~PipeChannel();

        void send(std::string data) override;
        std::string receive(uint32_t numBytes) override;
        void flush() override;

    protected:
        void open_pipe();
        void close_pipe();

        void read_thread();

    private:
        HANDLE pipe;
        std::vector<char> buffer;

        bool stop_thread;
        std::thread async_read_thread;
    };

    // basically just a static class
    class CCI_API_CLASS(StdOutChannel) : public Channel {
    public:
        void send(std::string data) override;
        std::string receive(uint32_t numBytes) override;
        void flush() override;
    };
}