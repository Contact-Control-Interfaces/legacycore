//
// Created by john_contactci on 2/22/2024.
//

#include <iostream>
#include <chrono>
#include "contactci.h"

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
    CciSessionHandle session = cci_create_session();
    bool s = cci_attach_haptic_state(session, false);

    HapticState *left;
    HapticState *right;

    bool r = cci_get_session_haptic_state(session, &left, &right);

    for (int i = 0; ; i++) {
        std::cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() << std::endl;
        spin(10);

        right->indexVibrationAmplitude = (i % 50 / 50.0f);
        cci_signal_session_haptic_state_changed(session);
    }

    cci_detach_haptic_state(session);
    cci_close_session(session);

    return 0;
}