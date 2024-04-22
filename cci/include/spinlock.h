//
// Created by john_contactci on 4/19/2024.
//

#pragma once

#include <atomic>

// Borrowed from https://rigtorp.se/spinlock/
namespace contactci {
    class SpinLock {
    public:
        void lock() noexcept;
        bool try_lock() noexcept;
        void unlock() noexcept;

    private:
        std::atomic<bool> lock_ = {0};
    };
}