#pragma once

// A contactci::Channel whose transport is two in-memory buffers: the bytes the
// library sent, and the bytes the "service" answers with. The framing and
// parsing code under test is the real Channel; only the pipe is replaced.

#include <cstdint>
#include <string>
#include <vector>

#include "cci/channel.h"
#include "cci/error.h"

namespace testsupport {
    class FakeChannel : public contactci::Channel {
    public:
        using contactci::Channel::HeaderSize;

        std::string sent;                    // everything the library wrote
        std::string inbound;                 // what receive() hands back, in order
        std::vector<uint32_t> receiveSizes;  // every size the library asked to read
        bool failNextReceive = false;        // the next receive() throws, as a broken pipe does

        void queue_response(int opcode, const std::string &body) {
            PacketHeader header;
            header.set_opcode(opcode);
            header.set_length(static_cast<uint32_t>(body.size()));
            inbound += header.SerializeAsString() + body;
        }

        // The first frame the library sent: its header, and its body.
        PacketHeader sent_header() const {
            PacketHeader header;
            header.ParseFromString(sent.substr(0, HeaderSize));
            return header;
        }
        std::string sent_body() const { return sent.substr(HeaderSize, sent_header().length()); }

    protected:
        bool is_connected() override { return true; }
        void send(std::string data) override { sent += data; }
        std::string receive(uint32_t numBytes) override {
            if (failNextReceive) {
                failNextReceive = false;
                throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_READ);
            }
            receiveSizes.push_back(numBytes);
            std::string out = inbound.substr(0, numBytes);
            inbound.erase(0, out.size());
            return out;
        }
        void flush() override {}
    };
}
