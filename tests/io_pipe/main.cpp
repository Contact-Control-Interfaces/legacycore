//
// Created by john_contactci on 8/4/2022.
//

#include <cstdio>

#include <channel.h>

using namespace contactci;

int main(int argc, char *argv[]) {
    io::PipeChannel pipeChannel;

    std::cout << "Sending dimension resume... " << std::endl;
    pipeChannel.send_dimension_resume_message( 10000, 20000, 300000);

    DimensionSuspendMessage message;
    std::string messageData = pipeChannel.receive_delimited();
    message.ParseFromString(messageData);

    std::cout << "d: " << message.dimension()
            << ", f: " << message.flags()
            << ", b: " << message.bitmask()
            << std::endl;

    return 0;
}