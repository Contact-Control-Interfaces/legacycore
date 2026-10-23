#include "posix_objects.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstring>
#include <ctime>
#include <random>
#include <stdexcept>

#include <fcntl.h>
#include <linux/futex.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace testsupport {
    namespace {
        std::runtime_error os_error(const std::string &what, const std::string &name) {
            return std::runtime_error(what + " failed for " + name + ": " + std::strerror(errno));
        }

        // Creates the object as the Service does (it owns every name; Core only
        // opens them) and maps it read-write.
        void *create_object(const std::string &name, std::size_t size) {
            int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
            if (fd < 0)
                throw os_error("shm_open", name);
            if (ftruncate(fd, static_cast<off_t>(size)) < 0) {
                close(fd);
                shm_unlink(name.c_str());
                throw os_error("ftruncate", name);
            }
            void *view = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            close(fd);
            if (view == MAP_FAILED) {
                shm_unlink(name.c_str());
                throw os_error("mmap", name);
            }
            return view;
        }

        void *map_sealed_object(int fd, std::size_t size) {
            void *view = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (view == MAP_FAILED) {
                close(fd);
                throw os_error("mmap", "a sealed object");
            }
            return view;
        }
    }

    int create_sealed_object(std::size_t size) {
        int fd = memfd_create("cci-test-session", MFD_CLOEXEC | MFD_ALLOW_SEALING);
        if (fd < 0)
            throw os_error("memfd_create", "a sealed object");
        if (ftruncate(fd, static_cast<off_t>(size)) < 0 ||
            fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL) < 0) {
            close(fd);
            throw os_error("ftruncate or F_ADD_SEALS", "a sealed object");
        }
        return fd;
    }

    std::string unique_name(const std::string &purpose) {
        static std::atomic<unsigned> counter{0};
        static const unsigned salt = std::random_device{}();   // a crashed run's leftovers never clash
        return "/cci-test-" + std::to_string(getpid()) + "-" + std::to_string(salt) + "-" +
               std::to_string(counter++) + "-" + purpose;
    }

    // ftruncate zero-fills: the counter starts at 0, as the contract requires.
    TestEvent::TestEvent(ObjectKind kind) {
        if (kind == ObjectKind::Sealed) {
            fd_ = create_sealed_object(sizeof(std::uint32_t));
            counter_ = static_cast<std::uint32_t *>(map_sealed_object(fd_, sizeof(std::uint32_t)));
        } else {
            name_ = unique_name("event");
            counter_ = static_cast<std::uint32_t *>(create_object(name_, sizeof(std::uint32_t)));
        }
    }

    TestEvent::~TestEvent() {
        munmap(counter_, sizeof(std::uint32_t));
        if (fd_ >= 0)
            close(fd_);
        else
            shm_unlink(name_.c_str());
    }

    // Contract §6, "Signal".
    void TestEvent::set() {
        __atomic_fetch_add(counter_, 1, __ATOMIC_RELEASE);
        syscall(SYS_futex, counter_, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
    }

    std::uint32_t TestEvent::count() const { return __atomic_load_n(counter_, __ATOMIC_ACQUIRE); }

    void TestEvent::store(std::uint32_t value) {
        __atomic_store_n(counter_, value, __ATOMIC_RELEASE);
        lastSeen_ = value;
    }

    // Contract §6, "Wait", with a relative timeout recomputed from one deadline.
    bool TestEvent::wait(int timeout_ms) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        for (;;) {
            std::uint32_t now = count();
            if (now != lastSeen_) {
                lastSeen_ = now;
                return true;
            }
            auto left = std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - std::chrono::steady_clock::now());
            if (left.count() <= 0)
                return false;
            timespec timeout{static_cast<time_t>(left.count() / 1000000000), static_cast<long>(left.count() % 1000000000)};
            if (syscall(SYS_futex, counter_, FUTEX_WAIT, lastSeen_, &timeout, nullptr, 0) < 0 &&
                errno != EAGAIN && errno != EINTR && errno != ETIMEDOUT)
                throw os_error("FUTEX_WAIT", name_);
        }
    }

    TestSharedMemory::TestSharedMemory(std::size_t size, ObjectKind kind) : size_(size) {
        if (kind == ObjectKind::Sealed) {
            fd_ = create_sealed_object(size);
            view_ = map_sealed_object(fd_, size);
        } else {
            name_ = unique_name("shm");
            view_ = create_object(name_, size);
        }
    }

    TestSharedMemory::~TestSharedMemory() {
        munmap(view_, size_);
        if (fd_ >= 0)
            close(fd_);
        else
            shm_unlink(name_.c_str());
    }
}
