//
// Created by john_contactci on 3/18/2024.
//

#include "cci/named_event.h"
#include "cci/error.h"

#include <windows.h>

using namespace contactci;

class NamedEvent::Implementation {
public:
    Implementation(const std::string &eventName, bool canWrite);
    ~Implementation();

    void set();
    void reset();

    bool wait(int timeout_ms);

private:
    HANDLE eventHandle;
};

NamedEvent::Implementation::Implementation(const std::string &eventName, bool canWrite) {
    DWORD eventAccess = SYNCHRONIZE;

    if (canWrite)
        eventAccess |= EVENT_MODIFY_STATE;

    eventHandle = OpenEvent(eventAccess, FALSE, eventName.c_str());

    if (eventHandle == nullptr)
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_OPEN);
}

NamedEvent::Implementation::~Implementation() {
    CloseHandle(eventHandle);
}

void NamedEvent::Implementation::set() {
    // Set and immediately unset event; this will wake all threads waiting on `eventHandle` and then they'll
    // Wait again after processing shared mem
    if (!SetEvent(eventHandle))
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_SET);
}

void NamedEvent::Implementation::reset() {
    // Set and immediately unset event; this will wake all threads waiting on `eventHandle` and then they'll
    // Wait again after processing shared mem
    if (!ResetEvent(eventHandle))
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_RESET);
}

bool NamedEvent::Implementation::wait(int timeout_ms) {
    DWORD result = WaitForSingleObject(eventHandle, timeout_ms);

    if (result == WAIT_FAILED)
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_WAIT);

    return result == WAIT_OBJECT_0;
}

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
