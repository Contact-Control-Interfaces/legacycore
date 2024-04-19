//
// Created by john_contactci on 4/19/2024.
//

#include "device.h"

using namespace contactci;

DeviceDescription::DeviceDescription(
    std::string product_line, std::string serial_number, bool is_right, bool is_connected
) : product_line(std::move(product_line)), serial_number(std::move(serial_number)),
    is_right(is_right), is_connected(is_connected) {}

const std::string &DeviceDescription::get_product_line() const {
    return product_line;
}

const std::string &DeviceDescription::get_serial_number() const {
    return serial_number;
}

bool DeviceDescription::get_is_right() const {
    return is_right;
}

bool DeviceDescription::get_is_connected() const {
    return is_connected;
}