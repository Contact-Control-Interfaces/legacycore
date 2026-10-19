//
// Created by zach_contactci on 10/13/2026.
//

#include "cci/shared_memory.h"
#include "cci/error.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace contactci;

// Core only ever opens regions the Service created; it never creates or unlinks them
// (communication/docs/linux-core-service-contract.md §4)
class SharedMemoryManager::Implementation {
public:
    Implementation(const std::string& memoryName, std::size_t size, bool canWrite);
    ~Implementation();

    void *get_memory();

private:
    void *sharedMemory;
    std::size_t size;
};

SharedMemoryManager::Implementation::Implementation(const std::string& memoryName, std::size_t size, bool canWrite)
        : size(size) {
    int fd = shm_open(memoryName.c_str(), canWrite ? O_RDWR : O_RDONLY, 0);

    if (fd < 0)
        throw contactci::Exception(CCI_ERR_SHM_FAILED_TO_OPEN);

    // Touching a mapping past the end of the object raises SIGBUS and would kill the Client, so refuse a short object
    struct stat info {};

    if (fstat(fd, &info) < 0 || static_cast<std::size_t>(info.st_size) < size) {
        close(fd);
        throw contactci::Exception(CCI_ERR_SHM_FAILED_TO_MAP);
    }

    int protection = canWrite ? PROT_READ | PROT_WRITE : PROT_READ;
    sharedMemory = mmap(nullptr, size, protection, MAP_SHARED, fd, 0);

    // The mapping keeps the object alive on its own
    close(fd);

    if (sharedMemory == MAP_FAILED)
        throw contactci::Exception(CCI_ERR_SHM_FAILED_TO_MAP);
}

SharedMemoryManager::Implementation::~Implementation() {
    munmap(sharedMemory, size);
}

void *SharedMemoryManager::Implementation::get_memory() {
    return sharedMemory;
}

SharedMemoryManager::SharedMemoryManager(const std::string& memoryName, std::size_t size, bool canWrite)
    : implementation(std::make_unique<SharedMemoryManager::Implementation>(memoryName, size, canWrite)) {}

SharedMemoryManager::~SharedMemoryManager() = default;

void *SharedMemoryManager::get_memory() {
    return implementation->get_memory();
}
