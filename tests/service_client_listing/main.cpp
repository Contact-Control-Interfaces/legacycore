#include <channel.h>

#include "info.pb.h"

using namespace contactci;

int main(int argc, char *argv[]) {
    io::PipeChannel pipeChannel;

    ClientListRequestMessage clientsRequest;

    pipeChannel.send_delimited(OpCode::opClientListRequestMessage, clientsRequest);

    ClientListResponseMessage clientsResponse;

    clientsResponse.ParseFromString(pipeChannel.receive_delimited());

    for (auto &client : clientsResponse.clients()) {
        std::cout << "Client:" << std::endl;
        std::cout << "\tProcess ID: " << client.processid() << std::endl;
        std::cout << "\tProcess Name: " << client.processname() << std::endl;
    }

    return 0;
}
