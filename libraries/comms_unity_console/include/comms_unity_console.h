//
// Created by john_contactci on 4/28/2022.
//

#ifndef CCI_COMMS_UNITY_CONSOLE_H
#define CCI_COMMS_UNITY_CONSOLE_H

#include <cci_lib_defs.h>

#include <comms/communicator.h>

#include <cstdint>
#include <sstream>

namespace contactci::comms {
    class CCI_API_CLASS(UnityConsoleCommunicator) : public contactci::comms::Communicator {
    public:
        explicit UnityConsoleCommunicator(void (*callback)(const char*, size_t));
        ~UnityConsoleCommunicator() = default;

        void start_frame() override;
        void end_frame() override;
        void send(uint8_t byte) override;

    private:
        void (*callback)(const char*, size_t);
        std::stringstream buffer;
    };
}

#endif //CCI_COMMS_STDOUT_H
