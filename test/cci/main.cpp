//
// Created by john_contactci on 2/22/2024.
//

#include <iostream>
#include <chrono>
#include "cci/session.h"
#include "cci/haptic_state.h"

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
    using namespace contactci;
    using namespace contactci::haptic_state;

    MutableHapticSession session;

    //.with_forcefeedback_amplitude(Hand::Right, Digit::Index, 0.5)
    //.with_forcefeedback_amplitude(Hand::Right, {Digit::Index, Digit::Middle}, 0.5)
    //.with_forcefeedback_amplitude(Hand::Left, {Digit::Thumb, Digit::Little}, 0.5)

    HapticStatePatch patch = HapticStatePatch()
        .with_forcefeedback_amplitude(
            for_digit(Hand::Right, Digit::Index), 0.5
        )
        .with_forcefeedback_amplitude(
            for_digits(Hand::Left, {Digit::Index, Digit::Middle}), 0.5
        )
        .with_vibration_amplitude(
            for_digits(Hand::Right, {Digit::Index, Digit::Middle}), 0.5
        );

    //session.apply_patch(patch);

    auto amplitude = patch.vibration_amplitude(Hand::Right, Digit::Index);
    auto amplitude2 = patch.vibration_amplitude(Hand::Right, Digit::Middle);

//    for (int i = 0; ; i++) {
//        std::cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() << std::endl;
//        spin(10);
//
//        stateManager.get_right_haptic_state().indexVibrationAmplitude = (i % 500 / 500.0f);
//        stateManager.get_right_haptic_state().indexForceFeedbackAmplitude = (i % 500 / 500.0f);
//
//        stateManager.signal_haptic_state_changed();
//    }

    return 0;
}