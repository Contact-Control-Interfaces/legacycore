// 
// Created by Jonathan Tainer on 6/1/2023
//

#include <haptics.h>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

void SetAllTargets(intptr_t ptr, float delta) {
	set_thumb_pid_target(ptr, delta);
	set_index_pid_target(ptr, delta);
	set_middle_pid_target(ptr, delta);
	set_ring_pid_target(ptr, delta);
	set_little_pid_target(ptr, delta);
}

int main() {
	intptr_t l = get_left_glove_pointer();
	intptr_t r = get_right_glove_pointer();

	while (true) {
		SetAllTargets(l, 10.f);
		SetAllTargets(r, -10.f);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		SetAllTargets(l, -10.f);
		SetAllTargets(r, 10.f);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	return 0;
}
