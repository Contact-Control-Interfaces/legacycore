//
// Created by john_contactci on 3/6/2024.
//

#include "haptics.h"

#include <cstdio>

int main() {
    start_maestro_detection_service();
    int right = get_right_glove_pointer();

    for (int i = 0; ; i++) {
        start_haptic_transaction(right);

        set_index_vibration_amplitude(right, ((100 + i) % 255) / 255.0f);

        end_haptic_transaction(right);

        getchar();
    }

    stop_maestro_detection_service();

    return 0;
}