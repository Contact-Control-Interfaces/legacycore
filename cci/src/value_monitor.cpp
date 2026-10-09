//
// Created by john_contactci on 4/19/2024.
//

#include <vector>
#include <string>
#include "cci/value_monitor.h"
#include "cci/session.h"

using namespace contactci;

// The monitor thread is started last in the constructor and stopped first in
// the destructor, so it never runs while this object is incomplete.
DeviceMonitor::DeviceMonitor(PipeChannel &channel, const std::string &eventName)
    : EventDrivenValueMonitor<std::vector<contactci::DeviceDescription>>(channel, eventName) {
    start();
}

DeviceMonitor::~DeviceMonitor() {
    stop();
}

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

    return result;
}

// The last connected device on that side, as before.
std::optional<contactci::DeviceDescription> DeviceMonitor::connected_device(bool isRight) {
    std::optional<contactci::DeviceDescription> found;

    for (auto &device : get_value()) {
        if (device.get_is_connected() && device.get_is_right() == isRight)
            found = device;
    }

    return found;
}

std::optional<contactci::DeviceDescription> DeviceMonitor::get_left_device() {
    return connected_device(false);
}

std::optional<contactci::DeviceDescription> DeviceMonitor::get_right_device() {
    return connected_device(true);
}

ClientMonitor::ClientMonitor(PipeChannel &channel, const std::string &eventName)
    : EventDrivenValueMonitor<std::vector<contactci::ClientDescription>>(channel, eventName) {
    start();
}

ClientMonitor::~ClientMonitor() {
    stop();
}

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
