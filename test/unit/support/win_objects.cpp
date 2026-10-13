#include "win_objects.h"

#include <atomic>
#include <stdexcept>

namespace testsupport {
    std::string unique_name(const std::string &purpose) {
        static std::atomic<unsigned> counter{0};
        return "Local\\cci-test-" + std::to_string(GetCurrentProcessId()) + "-" +
               std::to_string(counter++) + "-" + purpose;
    }

    TestEvent::TestEvent() : name_(unique_name("event")) {
        handle_ = CreateEventA(nullptr, TRUE, FALSE, name_.c_str());
        if (handle_ == nullptr)
            throw std::runtime_error("CreateEvent failed for " + name_);
    }

    TestEvent::~TestEvent() { CloseHandle(handle_); }

    void TestEvent::set() { SetEvent(handle_); }

    void TestEvent::reset() { ResetEvent(handle_); }

    bool TestEvent::is_set() const { return WaitForSingleObject(handle_, 0) == WAIT_OBJECT_0; }

    TestSharedMemory::TestSharedMemory(std::size_t size) : name_(unique_name("shm")) {
        mapping_ = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                                      static_cast<DWORD>(size), name_.c_str());
        if (mapping_ == nullptr)
            throw std::runtime_error("CreateFileMapping failed for " + name_);
        view_ = MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, size);
        if (view_ == nullptr)
            throw std::runtime_error("MapViewOfFile failed for " + name_);
    }

    TestSharedMemory::~TestSharedMemory() {
        UnmapViewOfFile(view_);
        CloseHandle(mapping_);
    }
}
