//
// Created by john_contactci on 3/18/2024.
//

#include "named_event.h"
#include "winerr_util.h"

using namespace contactci;

NamedEvent::NamedEvent(const std::string& eventName, bool canWrite) {
    DWORD eventAccess = SYNCHRONIZE;

    if (canWrite)
        eventAccess |= EVENT_MODIFY_STATE;

    eventHandle = OpenEvent(eventAccess, FALSE, eventName.c_str());

    if (eventHandle == nullptr) {
        throw std::runtime_error(
                std::string("Failed to open event: OpenEvent; GetLastError = ")
                + GetLastErrorAsString()
        );
    }
}

NamedEvent::~NamedEvent() {
    CloseHandle(eventHandle);
}

void NamedEvent::set() {
    // Set and immediately unset event; this will wake all threads waiting on `eventHandle` and then they'll
    // Wait again after processing shared mem
    if (!SetEvent(eventHandle)) {
        throw std::runtime_error(
                std::string("Failed to signal event: SetEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }
}

void NamedEvent::reset() {
    // Set and immediately unset event; this will wake all threads waiting on `eventHandle` and then they'll
    // Wait again after processing shared mem
    if (!ResetEvent(eventHandle)) {
        throw std::runtime_error(
                std::string("Failed to reset event: ResetEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }
}

bool NamedEvent::wait(int timeout_ms) {
    DWORD result = WaitForSingleObject(eventHandle, timeout_ms);

    if (result == WAIT_FAILED) {
        throw std::runtime_error(
                std::string("Failed to wait on event: WaitForSingleObject; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }

    return result == WAIT_OBJECT_0;
}