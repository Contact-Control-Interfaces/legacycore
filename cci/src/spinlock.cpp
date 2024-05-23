//
// Created by john_contactci on 4/19/2024.
//
#include "cci/spinlock.h"

// If compiling with MSVC, use the intel pause instruction intrinsic directly
// Otherwise, assume GCC and use their builtin
// These should both emit a "pause" instruction, and `_mm_pause` may be available on GCC as well; unsure
#if defined(_MSC_VER)
#include <emmintrin.h>
#define cci_pause() _mm_pause();
#else
#define cci_pause() __builtin_ia32_pause();
#endif

using namespace contactci;

// Borrowed from https://rigtorp.se/spinlock/
void SpinLock::lock() noexcept {
    for (;;) {
        // Optimistically assume the lock is free on the first try
        if (!lock_.exchange(true, std::memory_order_acquire)) {
            return;
        }
        // Wait for lock to be released without generating cache misses
        while (lock_.load(std::memory_order_relaxed)) {
            // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
            // hyper-threads
            cci_pause();
        }
    }
}

bool SpinLock::try_lock() noexcept {
    // First do a relaxed load to check if lock is free in order to prevent
    // unnecessary cache misses if someone does while(!try_lock())
    return !lock_.load(std::memory_order_relaxed) &&
           !lock_.exchange(true, std::memory_order_acquire);
}

void SpinLock::unlock() noexcept {
    lock_.store(false, std::memory_order_release);
}