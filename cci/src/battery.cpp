#include "cci/battery.h"

using namespace contactci;

BatteryStatusDescription::BatteryStatusDescription(std::string serial_number, float level, float minutes_remaining)
    : serial_number(std::move(serial_number)), level(level), minutes_remaining(minutes_remaining) {}

const std::string &BatteryStatusDescription::get_serial_number() const {
    return serial_number;
}

float BatteryStatusDescription::get_level() const {
    return level;
}

float BatteryStatusDescription::get_minutes_remaining() const {
    return minutes_remaining;
}
