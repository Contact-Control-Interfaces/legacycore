//
// Created by john_contactci on 2/21/2024.
//
#pragma once

#include <string>
#include <memory>

#ifndef _WIN32
#include "unique_fd.h"
#endif

namespace contactci {
    class SharedMemoryManager {
    public:
        SharedMemoryManager(const std::string& memoryName, std::size_t size, bool canWrite);
#ifndef _WIN32
        // An object the Service passed as a descriptor instead of a name, which must be exactly `size` bytes. The
        // descriptor is closed once it is mapped
        SharedMemoryManager(UniqueFd object, std::size_t size, bool canWrite);
#endif
        ~SharedMemoryManager();

        void *get_memory();

    private:
        class Implementation;
        std::unique_ptr<Implementation> implementation;
    };
}
