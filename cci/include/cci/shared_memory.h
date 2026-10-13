//
// Created by john_contactci on 2/21/2024.
//
#pragma once

#include <string>
#include <memory>

namespace contactci {
    class SharedMemoryManager {
    public:
        SharedMemoryManager(const std::string& memoryName, std::size_t size, bool canWrite);
        ~SharedMemoryManager();

        void *get_memory();

    private:
        class Implementation;
        std::unique_ptr<Implementation> implementation;
    };
}
