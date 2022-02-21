//
// Created by john_contactci on 2/21/2022.
//

#pragma once

#include <cstdint>

namespace contactci::comms {
    class Communicator {
    public:
        virtual void start_frame() = 0;

        virtual void end_frame() = 0;

        virtual void send(uint8_t byte) = 0;
    };
}
