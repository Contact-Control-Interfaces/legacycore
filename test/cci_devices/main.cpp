//
// Created by john_contactci on 2/22/2024.
//

#include <iostream>
#include <chrono>
#include "cci/session.h"

#include <windows.h>

using namespace std::chrono;

int main() {
    contactci::MutableHapticSession session;

    for (int i = 0; ; i++) {
        std::cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() << std::endl;
        Sleep(500);

        auto left = session.get_left_device();
        auto right = session.get_right_device();

        if (left.has_value())
            std::cout << "left: " << left.value().get_serial_number() << std::endl;

        if (right.has_value())
            std::cout << "right: " << right.value().get_serial_number() << std::endl;
    }

    return 0;
}