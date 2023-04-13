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


    private:
        HANDLE pipe;
    };

    // basically just a static class
    class CCI_API_CLASS(StdOutChannel) : public Channel {
    public:
        void send(std::string data) override;
        std::string receive(uint32_t numBytes) override;
    };
}