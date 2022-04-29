//
// Created by john_contactci on 4/28/2022.
//

#ifndef CCI_COMMS_STDOUT_H
#define CCI_COMMS_STDOUT_H

#include <cci_lib_defs.h>

#include <comms/communicator.h>

#include <cstdint>
#include <iostream>

namespace contactci::comms {
    class CCI_API_CLASS(StdoutCommunicator) : public contactci::comms::Communicator {
    public:
        StdoutCommunicator() = default;
        ~StdoutCommunicator() = default;

        void start_frame() override;
        void end_frame() override;
        void send(uint8_t byte) override;
    };
}

#endif //CCI_COMMS_STDOUT_H
