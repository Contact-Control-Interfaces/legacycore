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

void SetAllAmplitudes(intptr_t ptr, uint8_t amplitude) {
    set_thumb_motor_amplitude(ptr, amplitude);
    set_index_motor_amplitude(ptr, amplitude);
    set_middle_motor_amplitude(ptr, amplitude);
    set_ring_motor_amplitude(ptr, amplitude);
    set_little_motor_amplitude(ptr, amplitude);
}

void SetAllVibrationEffects(intptr_t ptr, uint8_t effect, uint8_t modifier){
    set_thumb_vibration_effect(ptr, effect, modifier);
    set_index_vibration_effect(ptr, effect, modifier);
    set_middle_vibration_effect(ptr, effect, modifier);
    set_ring_vibration_effect(ptr, effect, modifier);
    set_little_vibration_effect(ptr, effect, modifier);
}

void SetHaptics(intptr_t ptr){
    SetAllAmplitudes(ptr, 128);
    SetAllVibrationEffects(ptr, 1, 0);
}

void Update() {
    static intptr_t left = get_left_glove_pointer();
    static intptr_t right = get_right_glove_pointer();

    bool leftConnected = is_glove_connected(left);
    bool rightConnected = is_glove_connected(right);
    bool processing = is_ble_processing();

    std::cout << "[ " << (leftConnected ? 1 : 0);
    std::cout << " " << (rightConnected ? 1 : 0);
    std::cout << " ] " << (processing ? "p" : "");

    SetHaptics(left);
    SetHaptics(right);
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
