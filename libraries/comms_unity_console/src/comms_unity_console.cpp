//
// Created by john_contactci on 4/28/2022.
//

#include "comms_unity_console.h"

using namespace contactci::comms;

UnityConsoleCommunicator::UnityConsoleCommunicator(void (*callback)(const char*, size_t))
    : callback(callback) { }

void UnityConsoleCommunicator::start_frame() {
    buffer.str("");
    buffer.clear();
}

void UnityConsoleCommunicator::end_frame() {
    std::string str = buffer.str();

    callback(str.c_str(), str.length());
}

void UnityConsoleCommunicator::send(uint8_t byte) {
    buffer << (int)byte << '\t';
}
