// Linux only: Core's change signal and shared memory (named_event_posix.cpp,
// shared_memory_posix.cpp) against objects this test creates the way the
// Service does (support/posix_objects.h). The signal is a futex sequence
// counter (docs/adr/0001, communication/docs/linux-core-service-contract.md §6),
// so it is tested for what a Windows pulse can't promise: no change is lost.

#include <atomic>
#include <cerrno>
#include <chrono>
#include <thread>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include "cci/error.h"
#include "cci/named_event.h"
#include "cci/shared_memory.h"
#include "posix_objects.h"

using namespace std::chrono_literals;
using testsupport::TestEvent;
using testsupport::TestSharedMemory;

namespace {
    using Clock = std::chrono::steady_clock;

    template <typename Action>
    CciStatus status_of(Action action) {
        try {
            action();
        } catch (const contactci::Exception &e) {
            return e.get_error_code();
        }
        return CCI_SUCCESS;
    }
}

TEST(PosixChangeSignal, SetIncrementsTheCounterOnceAndResetLeavesIt) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), true);

    event.set();
    EXPECT_EQ(serviceSide.count(), 1u);
    event.reset();
    EXPECT_EQ(serviceSide.count(), 1u) << "reset() must neither undo nor repeat the signal";
    event.set();
    EXPECT_EQ(serviceSide.count(), 2u);
}

// The reason for ADR-0001: a change made while nobody waits is still seen.
TEST(PosixChangeSignal, AChangeMadeWhileNobodyWaitsIsNotLost) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), false);

    serviceSide.set();
    serviceSide.set();
    serviceSide.set();

    EXPECT_TRUE(event.wait(0)) << "the changes made before the wait were lost";
    EXPECT_FALSE(event.wait(10)) << "three changes before one wait are one wake; nothing is left pending";
}

// Contract §6 asks the loopback test to confirm that a futex wait works on the
// read-only mapping Core uses for the Service's counters.
TEST(PosixChangeSignal, AWaiterBlockedOnAReadOnlyMappingIsWokenByTheService) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), false);   // O_RDONLY, PROT_READ

    auto start = Clock::now();
    std::thread service([&] {
        std::this_thread::sleep_for(200ms);
        serviceSide.set();
    });
    bool woken = event.wait(10000);
    auto elapsed = Clock::now() - start;
    service.join();

    EXPECT_TRUE(woken);
    EXPECT_GE(elapsed, 200ms) << "returned before the Service signalled";
    EXPECT_LT(elapsed, 5s) << "only the timeout ended the wait: FUTEX_WAKE did not reach it";
}

TEST(PosixChangeSignal, CoresSignalWakesAServiceBlockedOnTheCounter) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), true);

    std::atomic<bool> woken{false};
    auto start = Clock::now();
    std::thread service([&] { woken = serviceSide.wait(10000); });
    std::this_thread::sleep_for(200ms);
    event.set();
    service.join();

    // The Service maps the counter at a different address, so a private futex would never reach it.
    EXPECT_TRUE(woken);
    EXPECT_LT(Clock::now() - start, 5s) << "only the timeout ended the Service's wait";
}

TEST(PosixChangeSignal, ATimeoutWithoutAChangeLastsTheWholeTimeout) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), false);

    auto start = Clock::now();
    EXPECT_FALSE(event.wait(300));
    auto elapsed = Clock::now() - start;

    EXPECT_GE(elapsed, 299ms);
    EXPECT_LT(elapsed, 5s);
}

TEST(PosixChangeSignal, EveryWaitingThreadSeesTheSameChange) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), false);

    std::atomic<int> woken{0};
    auto waiter = [&] { if (event.wait(10000)) woken++; };
    std::thread first(waiter), second(waiter);
    std::this_thread::sleep_for(200ms);
    serviceSide.set();
    first.join();
    second.join();

    EXPECT_EQ(woken.load(), 2);
}

// A thread's first wait starts from the value the event was opened with, so a
// monitor thread started after a change still sees it.
TEST(PosixChangeSignal, AThreadsFirstWaitSeesChangesSinceTheEventWasOpened) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), false);
    serviceSide.set();

    bool woken = false;
    std::thread late([&] { woken = event.wait(0); });
    late.join();

    EXPECT_TRUE(woken);
}

TEST(PosixChangeSignal, TheCounterWrapsAround) {
    TestEvent serviceSide;
    serviceSide.store(0xFFFFFFFFu);
    contactci::NamedEvent reader(serviceSide.name(), false);
    contactci::NamedEvent writer(serviceSide.name(), true);

    writer.set();

    EXPECT_EQ(serviceSide.count(), 0u);
    EXPECT_TRUE(reader.wait(0));
}

// Touching a mapping past the end of the object would SIGBUS the Client.
TEST(PosixChangeSignal, AnObjectSmallerThanTheCounterFailsToOpen) {
    TestSharedMemory tooSmall(2);
    EXPECT_EQ(status_of([&] { contactci::NamedEvent event(tooSmall.name(), false); }),
              CCI_ERR_EVENT_FAILED_TO_OPEN);
}

// Contract §4: Core only ever opens what the Service created.
TEST(PosixSharedMemory, OpeningAMissingRegionDoesNotCreateIt) {
    std::string name = testsupport::unique_name("never-created");

    EXPECT_EQ(status_of([&] { contactci::SharedMemoryManager memory(name, 2 * sizeof(HapticState), true); }),
              CCI_ERR_SHM_FAILED_TO_OPEN);

    int fd = shm_open(name.c_str(), O_RDONLY, 0);
    int error = errno;
    if (fd >= 0) {
        close(fd);
        shm_unlink(name.c_str());
    }
    EXPECT_LT(fd, 0) << "Core created " << name;
    EXPECT_EQ(error, ENOENT);
}

TEST(PosixSharedMemory, EveryByteOfTheRegionIsSharedBothWays) {
    TestSharedMemory serviceSide;
    ASSERT_EQ(serviceSide.size(), 144u);
    contactci::SharedMemoryManager memory(serviceSide.name(), serviceSide.size(), true);
    auto *service = static_cast<unsigned char *>(serviceSide.data());
    auto *core = static_cast<unsigned char *>(memory.get_memory());

    for (std::size_t i = 0; i < 144; ++i)
        service[i] = static_cast<unsigned char>(i);
    for (std::size_t i = 0; i < 144; ++i)
        ASSERT_EQ(core[i], i) << "byte " << i;

    for (std::size_t i = 0; i < 144; ++i)
        core[i] = static_cast<unsigned char>(255 - i);
    for (std::size_t i = 0; i < 144; ++i)
        ASSERT_EQ(service[i], 255 - i) << "byte " << i;
}
