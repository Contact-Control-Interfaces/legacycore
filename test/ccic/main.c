//
// Created by john_contactci on 2/22/2024.
//

#include <stdio.h>
#include <time.h>
#include "contactci.h"

void spin(long long start, int milliseconds){
    struct timespec time;
    long long current = start;

    do {
        timespec_get(&time, TIME_UTC);
        current = (time.tv_sec * 1000) + (time.tv_nsec / 1000000);
    } while(current - start < milliseconds);
}

int main() {
    CciSessionHandle session;
    cci_create_mutable_haptic_session(&session);

    HapticState *left;
    HapticState *right;

    cci_get_session_haptic_state(session, &left, &right);

    struct timespec time;
    long long current_millis = 0;

    for (int i = 0; ; i++) {
        timespec_get(&time, TIME_UTC);
        current_millis = (time.tv_sec * 1000) + (time.tv_nsec / 1000000);
        printf("%d\n", current_millis);
        fflush(stdout);

        spin(current_millis, 1000);

        //right->middleVibrationAmplitude = 0.5f + ((i % 50) / 100.0f);
        right->indexForceFeedbackAmplitude = i % 2 == 0 ? 0.5f : 0.0f;
        cci_signal_session_haptic_state_changed(session);
    }

    cci_close_session(session);

    return 0;
}