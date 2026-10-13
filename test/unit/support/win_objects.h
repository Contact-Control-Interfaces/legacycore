#pragma once

// Named Windows objects of the kind the service creates for its clients: a
// manual-reset event and a pagefile-backed shared memory section. Each gets a
// unique Local\ name, so tests never collide with each other or with a real
// Contact CI service running on the same machine.

#include <cstddef>
#include <string>
#include <windows.h>

#include <ccic/haptic_state.h>

namespace testsupport {
    std::string unique_name(const std::string &purpose);

    class TestEvent {
    public:
        TestEvent();
        ~TestEvent();
        TestEvent(const TestEvent &) = delete;
        TestEvent &operator=(const TestEvent &) = delete;

        const std::string &name() const { return name_; }
        HANDLE handle() const { return handle_; }

        void set();
        void reset();
        bool is_set() const;

    private:
        std::string name_;
        HANDLE handle_;
    };

    // Sized for the service's layout: a left and a right HapticState, back to back.
    class TestSharedMemory {
    public:
        explicit TestSharedMemory(std::size_t size = 2 * sizeof(HapticState));
        ~TestSharedMemory();
        TestSharedMemory(const TestSharedMemory &) = delete;
        TestSharedMemory &operator=(const TestSharedMemory &) = delete;

        const std::string &name() const { return name_; }
        void *data() const { return view_; }
        HapticState *left() const { return static_cast<HapticState *>(view_); }
        HapticState *right() const { return static_cast<HapticState *>(view_) + 1; }

    private:
        std::string name_;
        HANDLE mapping_;
        void *view_;
    };
}
