//
// Created by john_contactci on 3/18/2024.
//

#include "cci/named_event.h"
#include "cci/error.h"

using namespace contactci;

NamedEvent::NamedEvent(const std::string& eventName, bool canWrite) {
    DWORD eventAccess = SYNCHRONIZE;

    if (canWrite)
        eventAccess |= EVENT_MODIFY_STATE;

    eventHandle = OpenEvent(eventAccess, FALSE, eventName.c_str());

    if (eventHandle == nullptr)
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_OPEN);
}

NamedEvent::NamedEvent(contactci::NamedEvent &&other) : eventHandle(other.eventHandle) {
    other.eventHandle = nullptr;
}

NamedEvent::~NamedEvent() {
    CloseHandle(eventHandle);
}

void NamedEvent::set() {
    // Set and immediately unset event; this will wake all threads waiting on `eventHandle` and then they'll
    // Wait again after processing shared mem
    if (!SetEvent(eventHandle))
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_SET);
}

void NamedEvent::reset() {
    // Set and immediately unset event; this will wake all threads waiting on `eventHandle` and then they'll
    // Wait again after processing shared mem
    if (!ResetEvent(eventHandle))
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_RESET);
}

bool NamedEvent::wait(int timeout_ms) {
    DWORD result = WaitForSingleObject(eventHandle, timeout_ms);

    if (result == WAIT_FAILED)
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_WAIT);

    return result == WAIT_OBJECT_0;
}