#include <channel.h>

#include "info.pb.h"

using namespace contactci;

int main(int argc, char *argv[]) {
    io::PipeChannel pipeChannel;

    DeviceListRequestMessage devicesRequest;

    pipeChannel.send_delimited(OpCode::opDeviceListRequestMessage, devicesRequest);

    DeviceListResponseMessage devicesResponse;

    devicesResponse.ParseFromString(pipeChannel.receive_delimited());

    for (auto &device : devicesResponse.devices()) {
        std::cout << "Device:" << std::endl;
        std::cout << "\tProduct: " << device.productline() << std::endl;
        std::cout << "\tSerial Number: " << device.serialnumber() << std::endl;
        std::cout << "\tHandedness: " << (device.isright() ? "right" : "left") << std::endl;
        std::cout << "\tConnected: " << device.isconnected() << std::endl;
    }

    return 0;
}
