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
    auto devices = session.get_device_list();
    auto devices2 = session.get_device_list();
    auto devices3 = session.get_device_list();

    contactci::MutableHapticStateManager &stateManager = session.get_session_haptic_state();

    contactci::haptic_state::HapticStatePatch patch = contactci::haptic_state::HapticStatePatch()
        .with_forcefeedback<contactci::haptic_state::Digit::Index>(
            contactci::haptic_state::ff_amplitude(0.5)
        )
        .with_vibration<contactci::haptic_state::Digit::Index>(
            contactci::haptic_state::vibration_amplitude(0.6)
        );

    for (int i = 0; ; i++) {
        std::cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() << std::endl;
        spin(10);

        stateManager.get_right_haptic_state().indexVibrationAmplitude = (i % 500 / 500.0f);
        stateManager.get_right_haptic_state().indexForceFeedbackAmplitude = (i % 500 / 500.0f);

        stateManager.signal_haptic_state_changed();
    }

    return 0;
}