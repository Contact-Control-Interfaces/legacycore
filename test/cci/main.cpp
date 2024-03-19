//
// Created by john_contactci on 2/22/2024.
//

#include <iostream>
#include <chrono>
#include <session.h>

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
    contactci::Session session;

    session.attach_haptic_state(false);
    std::weak_ptr<contactci::HapticStateManager> weakStateManager = session.get_session_haptic_state();

    for (int i = 0; ; i++) {
        std::cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() << std::endl;
        spin(10);

        std::shared_ptr<contactci::HapticStateManager> stateManager = weakStateManager.lock();

        // If the session has been closed or state was detached then the weak_ptr::lock will return an empty shared_ptr
        if (!stateManager)
            break;

        stateManager->get_right_haptic_state().indexVibrationAmplitude = (i % 50 / 50.0f);

        stateManager->signal_haptic_state_changed();
    }

    return 0;
}