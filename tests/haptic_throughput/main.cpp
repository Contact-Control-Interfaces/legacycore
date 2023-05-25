#include <chrono>

#include <channel.h>

using namespace contactci;

typedef std::array<std::chrono::steady_clock::time_point, 2> (*measured_function)();
long long measure(measured_function);
void measure(measured_function, int repeats);

io::PipeChannel channel;

std::array<std::chrono::steady_clock::time_point, 2> update_haptics() {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    channel.send_force_feedback_update_message(false, 50, 255);
    channel.send_vibration_update_message(false, 52, 0, 255);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    return { start, end };
}

int main(int argc, char *argv[]) {
    int repeats = 1000;

    update_haptics();

    std::cout << "Measuring updating force feedback..." << std::endl;

    measure(update_haptics, repeats);
    std::cout << std::endl;

    return 0;
}

void output_iteration(long long duration){
    static int iteration = 1;

    std::cout << iteration << ":\t"
        << duration << " micros" << std::endl;

    iteration++;
}

void output_totals(long long total, int repeats){
    std::cout << "avg:\t" << total/repeats << " micros" << std::endl;
    std::cout << "total:\t" << total << " micros" << std::endl;
}

void measure(measured_function function, int repeats) {
    long long total = 0;

    for (int i = 0; i < repeats; i++) {
        long long iteration = measure(function);
        output_iteration(iteration);

        // update totals
        total += iteration;
    }

    output_totals(total, repeats);
}

long long measure(measured_function function) {
    std::array<std::chrono::steady_clock::time_point, 2> result = function();

    return std::chrono::duration_cast<std::chrono::microseconds>(result[1] - result[0]).count();
}
