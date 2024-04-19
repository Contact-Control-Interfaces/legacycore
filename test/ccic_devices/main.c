//
// Created by john_contactci on 2/22/2024.
//

#include <stdio.h>
#include <time.h>
#include "contactci.h"

#include "windows.h"

int main() {
    CciSessionHandle session = cci_create_mutable_haptic_session();

    struct timespec time;
    long long current_millis = 0;

    DeviceDescription left;
    DeviceDescription right;

    for (int i = 0; ; i++) {
        Sleep(500);

        timespec_get(&time, TIME_UTC);
        current_millis = (time.tv_sec * 1000) + (time.tv_nsec / 1000000);
        printf("%d\n", current_millis);

        if (cci_get_left_device(session, &left))
            printf("left: %s\n", left.serialNumber);

        if (cci_get_right_device(session, &right))
            printf("right: %s\n", right.serialNumber);

        fflush(stdout);
    }

    cci_close_session(session);

    return 0;
}