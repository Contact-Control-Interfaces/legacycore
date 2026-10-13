//
// Created by zach_contactci on 10/13/2026.
//

#include "cci/named_event.h"
#include "cci/error.h"

using namespace contactci;

// TODO(DEV-17): futex sequence counter in a small named shm object, see docs/adr/0001
class NamedEvent::Implementation {
public:
    Implementation(const std::string &eventName, bool canWrite) {
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_OPEN);
    }

    void set() {}
    void reset() {}

    bool wait(int timeout_ms) {
        return false;
    }
};

NamedEvent::NamedEvent(const std::string &eventName, bool canWrite)
    : implementation(std::make_unique<NamedEvent::Implementation>(eventName, canWrite)) {}

NamedEvent::NamedEvent(contactci::NamedEvent &&other) noexcept = default;

NamedEvent::~NamedEvent() = default;

void NamedEvent::set() {
    implementation->set();
}

void NamedEvent::reset() {
    implementation->reset();
}

bool NamedEvent::wait(int timeout_ms) {
    return implementation->wait(timeout_ms);
}
