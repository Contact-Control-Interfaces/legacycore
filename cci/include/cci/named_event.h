//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <string>
#include <memory>

#include "error.h"

namespace contactci {
    class NamedEvent {
    public:
        NamedEvent(const std::string &eventName, bool canWrite);
        NamedEvent(NamedEvent &&other) noexcept;
        ~NamedEvent();

        void set();
        void reset();

        bool wait(int timeout_ms);

    private:
        class Implementation;
        std::unique_ptr<Implementation> implementation;
    };
}
