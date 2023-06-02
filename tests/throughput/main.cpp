//
// Created by zach_contactci on 3/13/2023.
//

#include <chrono>

#include <channel.h>

using namespace contactci;

typedef std::array<std::chrono::steady_clock::time_point, 4> (*measured_function)();
std::array<long long, 3> measure(measured_function);
void measure(measured_function, int repeats);
std::string build_byte_array(int n);
std::string array_to_string(std::string& data);

std::string bytes_1;
std::string bytes_10;
std::string bytes_100;
std::string bytes_1k;
std::string bytes_10k;
std::string bytes_100k;
std::string bytes_1m;
std::string bytes_10m;
std::string bytes_100m;
uint32_t bitmask = 0x1F;

io::PipeChannel channel;

std::array<std::chrono::steady_clock::time_point, 4> dimension_resume_round_trip() {

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    // Send DimensionResume message
    channel.send_dimension_resume_message(100, 7, 255);

    // done sending
    std::chrono::steady_clock::time_point post_send = std::chrono::steady_clock::now();

    // Get DimensionSuspend response
    DimensionSuspendMessage message;
    std::string messageData = channel.receive_delimited();

    // done receiving
    std::chrono::steady_clock::time_point post_receive = std::chrono::steady_clock::now();

    message.ParseFromString(messageData);

    // end
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    return { start, post_send, post_receive, end };
}

std::array<std::chrono::steady_clock::time_point, 4> dimension_status_round_trip(uint32_t count, uint32_t bitmask, std::string data) {

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    channel.send_dimension_status_message(count, 2, bitmask);

    // done sending
    std::chrono::steady_clock::time_point post_send = std::chrono::steady_clock::now();

    // Get ReallyLong response
    DimensionStatusMessage message;
    std::string messageData = channel.receive_delimited();
    assert(messageData.length() >= count);

    // done receiving
    std::chrono::steady_clock::time_point post_receive = std::chrono::steady_clock::now();

    message.ParseFromString(messageData);

    // end
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    return { start, post_send, post_receive, end };
}

std::array<std::chrono::steady_clock::time_point, 4> dimension_status_round_trip_1(){
    return dimension_status_round_trip(1, bitmask, bytes_1);
}

std::array<std::chrono::steady_clock::time_point, 4> dimension_status_round_trip_10(){
    return dimension_status_round_trip(10, bitmask, bytes_10);
}

std::array<std::chrono::steady_clock::time_point, 4> dimension_status_round_trip_100(){
    return dimension_status_round_trip(100, bitmask, bytes_100);
}

std::array<std::chrono::steady_clock::time_point, 4> dimension_status_round_trip_1k(){
    return dimension_status_round_trip(1000, bitmask, bytes_1k);
}

std::array<std::chrono::steady_clock::time_point, 4> dimension_status_round_trip_10k(){
    return dimension_status_round_trip(10000, bitmask, bytes_10k);
}

std::array<std::chrono::steady_clock::time_point, 4> dimension_status_round_trip_100k(){
    return dimension_status_round_trip(100000, bitmask, bytes_100k);
}

std::array<std::chrono::steady_clock::time_point, 4> dimension_status_round_trip_1m(){
    return dimension_status_round_trip(1000000, bitmask, bytes_1m);
}

std::array<std::chrono::steady_clock::time_point, 4> dimension_status_round_trip_10m(){
    return dimension_status_round_trip(10000000, bitmask, bytes_10m);
}

std::array<std::chrono::steady_clock::time_point, 4> dimension_status_round_trip_100m(){
    return dimension_status_round_trip(100000000, bitmask, bytes_100m);
}

std::string array_to_string(std::string &data){
    std::string data_sent = "[ ";
    for (int i = 0 ; i < data.length(); i++){
        data_sent += std::to_string((int)data[i]) + ",";
    }
    data_sent += " ]";
    return data_sent;
}

std::string build_byte_array(int n){
    std::string result;
    for (int i = 0; i < n; i++){
        result.push_back((char)(i%256));
    }
    return result;
}

void toggle_logging(bool log){
    channel.send_dimension_suspend_message(log ? 1 : 0, 0, 0);
    std::string messageData = channel.receive_delimited();
    DimensionSuspendMessage message;
    message.ParseFromString(messageData);
}

int main(int argc, char *argv[]) {

    // Send/receive packet and reply first to make sure we're connected
    toggle_logging(false);

    // Build all needed byte arrays ahead of time so they aren't measured
    bytes_1 = build_byte_array(1);
    bytes_10 = build_byte_array(10);
    bytes_100 = build_byte_array(100);
    bytes_1k = build_byte_array(1 * 1000);
    bytes_10k = build_byte_array(10 * 1000);
    bytes_100k = build_byte_array(100 * 1000);
    bytes_1m = build_byte_array(1 * 1000 * 1000);
    bytes_10m = build_byte_array(10 * 1000 * 1000);
    bytes_100m = build_byte_array(100 * 1000 * 1000);

    int repeats = 100;

    /*std::cout << "Measuring DimensionResume (3 int32s) msg..." << std::endl;
    measure(dimension_resume_round_trip, repeats);
    std::cout << std::endl;*/

    std::cout << "Measuring DimensionStatus with 1 byte..." << std::endl;
    measure(dimension_status_round_trip_1, repeats);
    std::cout << std::endl;

    std::cout << "Measuring DimensionStatus with 10 bytes..." << std::endl;
    measure(dimension_status_round_trip_10, repeats);
    std::cout << std::endl;

    std::cout << "Measuring DimensionStatus with 100 bytes..." << std::endl;
    measure(dimension_status_round_trip_100, repeats);
    std::cout << std::endl;

    std::cout << "Measuring DimensionStatus with 1,000 bytes..." << std::endl;
    measure(dimension_status_round_trip_1k, repeats);
    std::cout << std::endl;

    std::cout << "Measuring DimensionStatus with 10,000 bytes..." << std::endl;
    measure(dimension_status_round_trip_10k, repeats);
    std::cout << std::endl;

    std::cout << "Measuring DimensionStatus with 100,000 bytes..." << std::endl;
    measure(dimension_status_round_trip_100k, repeats);
    std::cout << std::endl;

    /*std::cout << "Measuring DimensionStatus with 1,000,000 bytes..." << std::endl;
    measure(dimension_status_round_trip_1m, repeats);
    std::cout << std::endl;

    std::cout << "Measuring DimensionStatus with 10,000,000 bytes..." << std::endl;
    measure(dimension_status_round_trip_10m, repeats);
    std::cout << std::endl;

    std::cout << "Measuring DimensionStatus with 100,000,000 bytes..." << std::endl;
    measure(dimension_status_round_trip_100m, repeats);
    std::cout << std::endl;*/

    return 0;
}

void output_iteration(std::array<long long, 3> durations){
    static int iteration = 1;

    long long total_duration = durations[0] + durations[1] + durations[2];

    std::cout << iteration << ":\t"
        << total_duration << " micros"
        << "\t(send: \t" << durations[0]
        << ", \treceive: \t" << durations[1]
        << ", \tparse: \t" << durations[2]
        << ")" << std::endl;

    iteration++;
}

void output_totals(std::array<long long, 4> totals, int repeats){
    std::cout << "avg:\t" << totals[3]/repeats << " micros"
                << "\t(send: \t" << totals[0]/repeats
                << ", \treceive: \t" << totals[1]/repeats
                << ", \tparse: \t" << totals[2]/repeats
                << ")" << std::endl;
    std::cout << "total:\t" << totals[3] << " micros" << std::endl;
}

void measure(measured_function function, int repeats) {
    std::array<long long, 4> totals { 0, 0, 0, 0 };

    for (int i = 0; i < repeats; i++) {
        std::array<long long, 3> iteration = measure(function);
        output_iteration(iteration);

        // update totals
        totals[0] += iteration[0];
        totals[1] += iteration[1];
        totals[2] += iteration[2];
        totals[3] += iteration[0] + iteration[1] + iteration[2];
    }

    output_totals(totals, repeats);
}

std::array<long long, 3> measure(measured_function function) {
    std::array<std::chrono::steady_clock::time_point, 4> result = function();

    long long send_time = std::chrono::duration_cast<std::chrono::microseconds>(result[1] - result[0]).count();
    long long receive_time = std::chrono::duration_cast<std::chrono::microseconds>(result[2] - result[1]).count();
    long long parse_time = std::chrono::duration_cast<std::chrono::microseconds>(result[3] - result[2]).count();

    // Return microseconds between begin and end
    return { send_time, receive_time, parse_time };
}
