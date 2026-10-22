#pragma once

// The Linux counterparts of win_objects.h: the objects the Linux Service
// creates for its clients, implemented here from the contract
// (communication/docs/linux-core-service-contract.md §4 to §6), not from Core's
// own code, so a test that passes means both sides agree on it.
//
//   TestSharedMemory  a POSIX shm object, 144 bytes by default: a left and a
//                     right HapticState, back to back
//   TestEvent         a change signal: a 4-byte shm object whose uint32_t is a
//                     futex sequence counter (Core ADR-0001)
//
// Each object gets a unique name (unique_name()), so tests never collide with
// each other or with a real Service's /contactci-* objects, and is unlinked
// when it is destroyed.

#include <cstddef>
#include <cstdint>
#include <string>

#include <ccic/haptic_state.h>

namespace testsupport {
    // A valid POSIX shm name: one leading '/', no other '/'.
    std::string unique_name(const std::string &purpose);

    class TestEvent {
    public:
        TestEvent();
        ~TestEvent();
        TestEvent(const TestEvent &) = delete;
        TestEvent &operator=(const TestEvent &) = delete;

        const std::string &name() const { return name_; }

        // The Service's signal: increment the counter, then FUTEX_WAKE every waiter.
        void set();

        // Nothing: a counter has nothing to reset. Kept so the platform-neutral
        // tests, written against the Windows event, compile unchanged.
        void reset() {}

        // Waits, as the Service does, until the counter differs from the value
        // this object last returned from wait() (0 at first). True if it changed.
        bool wait(int timeout_ms);

        std::uint32_t count() const;
        void store(std::uint32_t value);   // to start the counter somewhere other than 0

    private:
        std::string name_;
        std::uint32_t *counter_;
        std::uint32_t lastSeen_ = 0;
    };

    class TestSharedMemory {
    public:
        explicit TestSharedMemory(std::size_t size = 2 * sizeof(HapticState));
        ~TestSharedMemory();
        TestSharedMemory(const TestSharedMemory &) = delete;
        TestSharedMemory &operator=(const TestSharedMemory &) = delete;

        const std::string &name() const { return name_; }
        std::size_t size() const { return size_; }
        void *data() const { return view_; }
        HapticState *left() const { return static_cast<HapticState *>(view_); }
        HapticState *right() const { return static_cast<HapticState *>(view_) + 1; }

    private:
        std::string name_;
        std::size_t size_;
        void *view_;
    };
}
