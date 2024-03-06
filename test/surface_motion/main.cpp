//
// Created by john_contactci on 3/6/2024.
//

#include "haptics.h"
#include <chrono>
#include <cstdio>
#include <iostream>

#include <windows.h>

using namespace std::chrono;

int main() {
    start_maestro_detection_service();
    int right = get_right_glove_pointer();

    for (int i = 0; ; i++) {
        std::cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() << std::endl;

        start_haptic_transaction(right);

        set_thumb_vibration_amplitude(right, (i % 50 / 50.0f));
        set_index_vibration_amplitude(right, (i % 50 / 50.0f));
        set_middle_vibration_amplitude(right, (i % 50 / 50.0f));
        set_ring_vibration_amplitude(right, (i % 50 / 50.0f));
        set_little_vibration_amplitude(right, (i % 50 / 50.0f));

        end_haptic_transaction(right);
    }

    stop_maestro_detection_service();

    return 0;
}