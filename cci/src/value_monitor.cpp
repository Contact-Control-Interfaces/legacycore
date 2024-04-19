//
// Created by john_contactci on 4/19/2024.
//

#include <vector>
#include <string>
#include "value_monitor.h"
#include "session.h"

using namespace contactci;

DeviceMonitor::DeviceMonitor(PipeChannel &channel, const std::string &eventName)
    : EventDrivenValueMonitor<std::vector<contactci::DeviceDescription>>(channel, eventName) {}

std::vector<contactci::DeviceDescription> DeviceMonitor::get_new_value() {
    DeviceListResponseMessage response = channel.get_device_list();

    std::vector<contactci::DeviceDescription> result;

    std::transform(
        std::begin(response.devices()), std::end(response.devices()),
        std::back_inserter(result),
        [](const DeviceDescriptionMessage &msg) {
            return contactci::DeviceDescription(
                    msg.productline(), msg.serialnumber(), msg.isright(), msg.isconnected()
            );
        }
    );

    leftDevice = std::nullopt;
    rightDevice = std::nullopt;

    for (auto &device : result) {
        if (device.get_is_connected()) {
            if (device.get_is_right())
                rightDevice = device;
            else
                leftDevice = device;
        }
    }

    return result;
}

std::optional<contactci::DeviceDescription> DeviceMonitor::get_left_device() {
    return leftDevice;
}

std::optional<contactci::DeviceDescription> DeviceMonitor::get_right_device() {
    return rightDevice;
}

ClientMonitor::ClientMonitor(PipeChannel &channel, const std::string &eventName)
    : EventDrivenValueMonitor<std::vector<contactci::ClientDescription>>(channel, eventName) {}

std::vector<contactci::ClientDescription> ClientMonitor::get_new_value() {
    ClientListResponseMessage response = channel.get_client_list();

    std::vector<contactci::ClientDescription> result;

    std::transform(
        std::begin(response.clients()), std::end(response.clients()),
        std::back_inserter(result),
        [](const ClientDescriptionMessage& msg) {
            return contactci::ClientDescription(msg.processid(), msg.processname());
        }
    );

    return result;
}