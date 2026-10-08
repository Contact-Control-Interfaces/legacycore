//
// Created by john_contactci on 2/22/2024.
//

#include <iostream>
#include <chrono>
#include "cci/session.h"

using namespace std::chrono;

void spin(int milliseconds){
    auto start = std::chrono::high_resolution_clock::now();
    long long dt;
    do {
        auto finish = std::chrono::high_resolution_clock::now();
        auto span = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
        dt = span.count();
    } while(dt < milliseconds);
}

int main() {
    contactci::MutableHapticSession session;

    contactci::MutableHapticStateManager &stateManager = session.get_session_haptic_state();

    for (int i = 0; ; i++) {
        std::cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() << std::endl;
        spin(10);

        stateManager.get_right_haptic_state().indexVibrationAmplitude = (i % 500 / 500.0f);

        stateManager.signal_haptic_state_changed();
    }

    return 0;
}