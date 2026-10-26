//
// Created by zach_contactci on 10/22/2026.
//

#pragma once

// POSIX only

#include <unistd.h>

namespace contactci {
    // Owns a file descriptor and closes it when destroyed, so a descriptor the Service passed is closed on every path,
    // including every exception
    class UniqueFd {
    public:
        explicit UniqueFd(int fd = -1) noexcept : fd(fd) {}
        UniqueFd(UniqueFd &&other) noexcept : fd(other.fd) { other.fd = -1; }
        UniqueFd(const UniqueFd &) = delete;
        UniqueFd &operator=(const UniqueFd &) = delete;

        UniqueFd &operator=(UniqueFd &&other) noexcept {
            if (this != &other) {
                reset();
                fd = other.fd;
                other.fd = -1;
            }

            return *this;
        }

        ~UniqueFd() { reset(); }

        int get() const { return fd; }

        void reset() {
            if (fd >= 0)
                close(fd);

            fd = -1;
        }

    private:
        int fd;
    };
}
