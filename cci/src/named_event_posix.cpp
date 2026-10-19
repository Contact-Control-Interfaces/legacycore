//
// Created by zach_contactci on 10/13/2026.
//

#include "cci/named_event.h"
#include "cci/shared_memory.h"
#include "cci/error.h"

#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <ctime>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

using namespace contactci;

// A futex sequence counter in a 4-byte shm object, not a pulse: a change that happens while the waiter is busy is
// never lost. See docs/adr/0001 and communication/docs/linux-core-service-contract.md §6
class NamedEvent::Implementation {
public:
    Implementation(const std::string &eventName, bool canWrite);

    void set();
    void reset();

    bool wait(int timeout_ms);

private:
    SharedMemoryManager counterMemory;
    std::uint32_t *counter;
    bool canWrite;
    // Per waiter: the counter value this event last returned for
    std::atomic<std::uint32_t> last;
};

NamedEvent::Implementation::Implementation(const std::string &eventName, bool canWrite) try
        : counterMemory(eventName, sizeof(std::uint32_t), canWrite),
          counter(static_cast<std::uint32_t *>(counterMemory.get_memory())),
          canWrite(canWrite),
          last(std::atomic_ref<std::uint32_t>(*counter).load(std::memory_order_acquire)) {
} catch (contactci::Exception &) {
    throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_OPEN);
}

void NamedEvent::Implementation::set() {
    // The counter is mapped read-only without write access, and writing through it would SIGSEGV the Client
    if (!canWrite)
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_SET);

    std::atomic_ref<std::uint32_t>(*counter).fetch_add(1, std::memory_order_release);

    // Shared between processes, so never FUTEX_PRIVATE_FLAG
    if (syscall(SYS_futex, counter, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0) < 0)
        throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_SET);
}

void NamedEvent::Implementation::reset() {
    // Nothing to undo: set() is a single increment, unlike the Win32 Set(); Reset(); pulse
}

bool NamedEvent::Implementation::wait(int timeout_ms) {
    std::atomic_ref<std::uint32_t> current(*counter);

    // Negative means wait forever, like INFINITE on Windows. Otherwise a single absolute deadline, so retries after a
    // spurious wake don't restart the timeout
    timespec deadline {};
    timespec *deadline_ptr = nullptr;

    if (timeout_ms >= 0) {
        clock_gettime(CLOCK_MONOTONIC, &deadline);
        deadline.tv_sec += timeout_ms / 1000;
        deadline.tv_nsec += static_cast<long>(timeout_ms % 1000) * 1000000;

        if (deadline.tv_nsec >= 1000000000) {
            deadline.tv_sec += 1;
            deadline.tv_nsec -= 1000000000;
        }

        deadline_ptr = &deadline;
    }

    std::uint32_t seen = last.load();

    for (;;) {
        std::uint32_t now = current.load(std::memory_order_acquire);

        if (now != seen) {
            last = now;
            return true;
        }

        // FUTEX_WAIT_BITSET because it takes an absolute CLOCK_MONOTONIC deadline
        if (syscall(SYS_futex, counter, FUTEX_WAIT_BITSET, seen, deadline_ptr, nullptr, FUTEX_BITSET_MATCH_ANY) == 0)
            continue;

        // EAGAIN: the counter moved before we slept. EINTR: a signal. Both re-check the counter
        if (errno == EAGAIN || errno == EINTR)
            continue;

        if (errno != ETIMEDOUT)
            throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_WAIT);

        // Deadline passed: one last check
        now = current.load(std::memory_order_acquire);

        if (now != seen) {
            last = now;
            return true;
        }

        return false;
    }
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
