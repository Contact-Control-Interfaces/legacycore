//
// Created by zach_contactci on 10/13/2026.
//

#include "cci/named_event.h"
#include "cci/shared_memory.h"
#include "cci/error.h"

#include <contactci/haptic_state.h>

#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <ctime>
#include <linux/futex.h>
#include <mutex>
#include <sys/syscall.h>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>

using namespace contactci;

// Identifies the calling thread to every NamedEvent. Unlike std::thread::id, it's never reused after the thread exits,
// so a new thread can't inherit an old one's last-seen counter. 64 bits never wrap in practice.
static std::uint64_t this_waiter() {
    static std::atomic<std::uint64_t> nextWaiter {0};
    thread_local const std::uint64_t waiter = nextWaiter.fetch_add(1, std::memory_order_relaxed);
    return waiter;
}

// A futex sequence counter in a 4-byte shm object, not a pulse: a change that happens while the waiter is busy is
// never lost. See docs/adr/0001 and communication/docs/linux-core-service-contract.md §6. The counter's type, size and
// initial value are communication's contactci/haptic_state.h (DEV-16).
static_assert(std::is_same_v<ContactCiChangeCounter, std::uint32_t>, "the change counter is a 32-bit futex word");
static_assert(CONTACTCI_CHANGE_COUNTER_SIZE == sizeof(std::uint32_t), "the change counter is a 32-bit futex word");
class NamedEvent::Implementation {
public:
    Implementation(const std::string &eventName, bool canWrite);
    Implementation(UniqueFd counterObject, bool canWrite);

    void set();
    void reset();

    bool wait(int timeout_ms);

private:
    std::uint32_t last_seen_by_this_thread();
    void remember_for_this_thread(std::uint32_t seen);

    SharedMemoryManager counterMemory;
    std::uint32_t *counter;
    bool canWrite;

    // Each waiting thread tracks the counter value it last returned, so every thread waiting on the same event sees
    // every change. A thread's first wait starts from the value read when the event was opened.
    std::uint32_t opened;
    std::mutex lastSeenLock;
    std::unordered_map<std::uint64_t, std::uint32_t> lastSeen;
};

NamedEvent::Implementation::Implementation(const std::string &eventName, bool canWrite) try
        : counterMemory(eventName, CONTACTCI_CHANGE_COUNTER_SIZE, canWrite),
          counter(static_cast<std::uint32_t *>(counterMemory.get_memory())),
          canWrite(canWrite),
          opened(std::atomic_ref<std::uint32_t>(*counter).load(std::memory_order_acquire)) {
} catch (contactci::Exception &) {
    throw contactci::Exception(CCI_ERR_EVENT_FAILED_TO_OPEN);
}

// Not rethrown as CCI_ERR_EVENT_FAILED_TO_OPEN: the contract fails a Session whose passed objects are the wrong size
// with CCI_ERR_SHM_FAILED_TO_OPEN, the counter included (§3)
NamedEvent::Implementation::Implementation(UniqueFd counterObject, bool canWrite)
        : counterMemory(std::move(counterObject), sizeof(std::uint32_t), canWrite),
          counter(static_cast<std::uint32_t *>(counterMemory.get_memory())),
          canWrite(canWrite),
          opened(std::atomic_ref<std::uint32_t>(*counter).load(std::memory_order_acquire)) {}

std::uint32_t NamedEvent::Implementation::last_seen_by_this_thread() {
    std::lock_guard<std::mutex> guard(lastSeenLock);
    auto entry = lastSeen.find(this_waiter());

    return entry == lastSeen.end() ? opened : entry->second;
}

void NamedEvent::Implementation::remember_for_this_thread(std::uint32_t seen) {
    std::lock_guard<std::mutex> guard(lastSeenLock);
    lastSeen[this_waiter()] = seen;
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

    // Never hold lastSeenLock across the futex wait: other threads must be able to start waiting meanwhile
    std::uint32_t seen = last_seen_by_this_thread();

    for (;;) {
        std::uint32_t now = current.load(std::memory_order_acquire);

        if (now != seen) {
            remember_for_this_thread(now);
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
            remember_for_this_thread(now);
            return true;
        }

        return false;
    }
}

NamedEvent::NamedEvent(const std::string &eventName, bool canWrite)
    : implementation(std::make_unique<NamedEvent::Implementation>(eventName, canWrite)) {}

NamedEvent::NamedEvent(UniqueFd counter, bool canWrite)
    : implementation(std::make_unique<NamedEvent::Implementation>(std::move(counter), canWrite)) {}

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
