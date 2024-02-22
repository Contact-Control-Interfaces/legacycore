//
// Created by john_contactci on 2/22/2024.
//

#include "haptics.h"

int main() {
    start_maestro_detection_service();

    start_haptic_transaction(get_left_glove_pointer());

    set_index_motor_amplitude(get_left_glove_pointer(), 100);

    end_haptic_transaction(get_left_glove_pointer());

    return 0;
}