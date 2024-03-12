//
// Created by john_contactci on 2/22/2024.
//

#include <iostream>
#include <chrono>
#include "haptics.h"

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
    start_maestro_detection_service();
    int right = get_right_glove_pointer();

    for (int i = 0; ; i++) {
        std::cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() << std::endl;
        spin(10);

        start_haptic_transaction(right);

        set_thumb_vibration_amplitude(right, (i % 50 / 50.0f));
        set_index_vibration_amplitude(right, (i % 50 / 50.0f));
        set_middle_vibration_amplitude(right, (i % 50 / 50.0f));
        set_ring_vibration_amplitude(right, (i % 50 / 50.0f));
        set_little_vibration_amplitude(right, (i % 50 / 50.0f));

        end_haptic_transaction(right);
    }

    return 0;
}