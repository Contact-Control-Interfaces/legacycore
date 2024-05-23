//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <string>
#include <stdexcept>
#include <windows.h>

#include "error.h"

namespace contactci {
    class NamedEvent {
    public:
        NamedEvent(const std::string &eventName, bool canWrite);
        NamedEvent(NamedEvent &&other);
        ~NamedEvent();

        void set();
        void reset();

        bool wait(int timeout_ms);

    private:
        HANDLE eventHandle;
    };
}
