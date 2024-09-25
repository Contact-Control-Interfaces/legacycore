//
// Created by john_contactci on 2/22/2024.
//

#include <iostream>
#include <chrono>
#include "cci/session.h"

#include <windows.h>

#include "../../ccic/include/contactci.h"

using namespace std::chrono;

int main() {
    std::cout << "testing wave cache..." << std::endl;
    contactci::MutableHapticSession session;

    Sleep(500);
    auto left = session.get_left_device();
    auto right = session.get_right_device();

        contactci::DeviceDescription *device;
        if (left.has_value()) {
            std::cout << "left: " << left.value().get_serial_number() << std::endl;
            device = &left.value();
        } else if (right.has_value()) {
            std::cout << "right: " << right.value().get_serial_number() << std::endl;
            device = &right.value();
        } else {
            std::cerr << "No devices connected" << std::endl;
            return 1;
        }

    std::vector<float> dummy;
    for (int i = 0; i < 255; i++) {
        dummy.push_back(static_cast<float>(i) / 255.f);
    }

    std::cout << "Transmitting waveform with " << dummy.size() << "values" << std::endl;
    auto &mgr = session.get_session_device_manager();

    CachedWaveform vout;
    auto res = mgr.transfer_waveform(*device, 8000, 0, dummy, vout, "testvalue");

    std::cout << "Transmit finished. Got result: ";

    switch (res) {
        case error:
            std::cout << "error";
            break;
        case indexConflict:
            std::cout << "idx conflict";
            break;
        case noSpace:
            std::cout << "no space";
            break;
        case okay:
            std::cout << "success";
            break;
        default: ;
    }

    std::cout << std::endl;

    std::cout << "requesting wave table.." << std::endl;

    auto table = mgr.get_cached_waveforms(*device, false);

    std::cout << "table contains " << table.size() << "entries" << std::endl;
    for(auto wav : table) {
        std::cout << "index: " << wav.index << " size: " << wav.length << " identifier: " << wav.identifier.value_or("NUL") << std::endl;
    }

    return 0;
}
