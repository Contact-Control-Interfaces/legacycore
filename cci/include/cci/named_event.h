//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <string>
#include <memory>

#include "error.h"

#ifndef _WIN32
#include "unique_fd.h"
#endif

namespace contactci {
    class NamedEvent {
    public:
        NamedEvent(const std::string &eventName, bool canWrite);
#ifndef _WIN32
        // A counter the Service passed as a descriptor instead of a name. The descriptor is closed once it is mapped
        NamedEvent(UniqueFd counter, bool canWrite);
#endif
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
