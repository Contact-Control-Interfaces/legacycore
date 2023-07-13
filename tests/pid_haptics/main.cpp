//
// Created by zach_contactci on 5/2/2023.
//

#include <haptics.h>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

typedef void (*Action)();

void PerformAction(const std::string& text, Action action){
    std::cout << text << "...";
    action();
    std::cout << "done." << std::endl;
}

void StartDetection() {
    start_maestro_detection_service();
}

void SetAllVibrationEffects(intptr_t ptr, uint8_t effect, uint8_t modifier){
    set_thumb_vibration_effect(ptr, effect, modifier);
    set_index_vibration_effect(ptr, effect, modifier);
    set_middle_vibration_effect(ptr, effect, modifier);
    set_ring_vibration_effect(ptr, effect, modifier);
    set_little_vibration_effect(ptr, effect, modifier);
}

void SetAllPIDTargets(intptr_t ptr, float target) {
    set_thumb_pid_target(ptr, target);
    set_index_pid_target(ptr, target);
    set_middle_pid_target(ptr, target);
    set_ring_pid_target(ptr, target);
    set_little_pid_target(ptr, target);
}

void SetHaptics(intptr_t ptr) {
    SetAllPIDTargets(ptr, 10);
    SetAllVibrationEffects(ptr, 52, 0);
}

void Update() {
    static intptr_t left = get_left_glove_pointer();
    static intptr_t right = get_right_glove_pointer();

    start_haptic_transaction(left);
    SetHaptics(left);
    end_haptic_transaction(left);

    start_haptic_transaction(right);
    SetHaptics(right);
    end_haptic_transaction(right);
}

int main(int argc, char *argv[]) {

    int fps = 90;

    PerformAction("Starting detection", StartDetection);

    int frame = 0;
    while (true) {
        frame++;
        PerformAction("Frame " + std::to_string(frame), Update);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000/fps));
    }

    return 0;
}
