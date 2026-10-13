#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "cci/spinlock.h"

TEST(SpinLock, TryLockFailsWhileHeld) {
    contactci::SpinLock lock;
    ASSERT_TRUE(lock.try_lock());
    EXPECT_FALSE(lock.try_lock());
    lock.unlock();
    EXPECT_TRUE(lock.try_lock());
    lock.unlock();
}

TEST(SpinLock, LockCanBeTakenAgainAfterUnlock) {
    contactci::SpinLock lock;
    lock.lock();
    lock.unlock();
    lock.lock();
    EXPECT_FALSE(lock.try_lock());
    lock.unlock();
}

// Channel uses this lock to keep each request paired with its response while
// the device and client monitors share one pipe.
TEST(SpinLock, ExcludesConcurrentWriters) {
    contactci::SpinLock lock;
    long counter = 0;   // deliberately not atomic: only the lock protects it
    constexpr int threads = 4, increments = 100000;

    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&] {
            for (int i = 0; i < increments; ++i) {
                lock.lock();
                ++counter;
                lock.unlock();
            }
        });
    }
    for (auto &w : workers) w.join();

    EXPECT_EQ(counter, static_cast<long>(threads) * increments);
}
