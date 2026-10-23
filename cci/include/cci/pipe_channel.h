//
// Created by john_contactci on 7/27/2022.
//

#pragma once

#include "channel.h"
#include "named_pipe.h"

#include <memory>
#include <vector>
#include <string>

#ifndef _WIN32
#include "unique_fd.h"

struct msghdr;   // <sys/socket.h>, kept out of this header
#endif

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

#ifndef _WIN32
        // The descriptors the Service attached to the session init reply (communication's
        // docs/linux-core-service-contract.md §3): this Session's haptic state region and counter when write access
        // was granted, otherwise none. Throws CCI_ERR_SHM_FAILED_TO_OPEN unless exactly `expected` arrived whole.
        // Call it once, straight after initialize_session(): descriptors on any later frame are closed as they arrive
        std::vector<UniqueFd> take_session_descriptors(std::size_t expected);
#endif

    private:
        std::unique_ptr<NamedPipe> pipe;

#ifndef _WIN32
        void keep_descriptors(::msghdr &message);

        std::vector<UniqueFd> sessionDescriptors;
        bool sessionDescriptorsTruncated = false;
        bool sessionDescriptorsTaken = false;
#endif
    };
 }