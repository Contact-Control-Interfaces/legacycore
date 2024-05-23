//
// Created by john_contactci on 7/27/2022.
//

#pragma once

#include "channel.h"
#include "named_pipe.h"

#include <windows.h>
#include <vector>
#include <string>

namespace contactci {

    class PipeChannel : public Channel {
    public:
        PipeChannel();
        PipeChannel(PipeChannel &&other);
        ~PipeChannel() = default;

        bool is_connected() override;
        void send(std::string data) override;
        std::string receive(uint32_t numBytes) override;
        void flush() override;

    private:
        std::unique_ptr<NamedPipe> pipe;
    };
 }