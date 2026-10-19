//
// Created by john_contactci on 4/19/2024.
//
#include "cci/spinlock.h"

// A spin-wait hint for the CPU: "pause" on x86, "yield" on ARM (Jetson).
// MSVC uses the intel pause intrinsic directly; GCC on x86 uses its builtin
#if defined(_MSC_VER)
#include <emmintrin.h>
#define cci_pause() _mm_pause();
#elif defined(__x86_64__) || defined(__i386__)
#define cci_pause() __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
#define cci_pause() __asm__ __volatile__("yield");
#else
#define cci_pause()
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