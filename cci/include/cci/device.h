//
// Created by john_contactci on 4/19/2024.
//

#pragma once

#include <ccic/lib_defs.h>

#include <string>


namespace contactci {
    class CCI_API_CLASS(DeviceDescription) {
    public:
        DeviceDescription(std::string product_line, std::string serial_number, bool is_right, bool is_connected);

        const std::string &get_product_line() const;
        const std::string &get_serial_number() const;

        bool get_is_right() const;
        bool get_is_connected() const;

    private:
        std::string product_line;
        std::string serial_number;
        bool is_right;
        bool is_connected;
    };
}