//
// Created by john_contactci on 2/22/2024.
//

#include <iostream>
#include <chrono>
#include "haptics.h"

using namespace std::chrono;

int main() {
    start_maestro_detection_service();

    for (int i = 0; ; i++) {
        std::cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() << std::endl;

        //start_haptic_transaction(get_left_glove_pointer());
        start_haptic_transaction(get_right_glove_pointer());

        //set_index_motor_amplitude(get_left_glove_pointer(), (100 + i) % 255);
        //set_index_vibration_amplitude(get_left_glove_pointer(), ((100 + i) % 255) / 255.0f);

        set_index_motor_amplitude(get_right_glove_pointer(), (100 + i) % 255);
        set_index_vibration_amplitude(get_right_glove_pointer(), ((100 + i) % 255) / 255.0f);

        //end_haptic_transaction(get_left_glove_pointer());
        end_haptic_transaction(get_right_glove_pointer());

        getchar();
    }

    return 0;
}