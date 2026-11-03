#pragma once

#include <ccic/lib_defs.h>

#include <string>

namespace contactci {
    // A device's battery status, from the Service's BatteryStatusDescriptionMessage.
    // A level or time of 0 means not available: a glove that is running never reports 0.
    class CCI_API_CLASS(BatteryStatusDescription) {
    public:
        BatteryStatusDescription(std::string serial_number, float level, float minutes_remaining);

        const std::string &get_serial_number() const;
        float get_level() const;               // 0.0 to 1.0
        float get_minutes_remaining() const;

    private:
        std::string serial_number;
        float level;
        float minutes_remaining;
    };
}
