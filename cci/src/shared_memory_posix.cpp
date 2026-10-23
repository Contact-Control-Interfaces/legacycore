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

// Core never creates or unlinks a region. It opens the global ones by the names the Service sends, and maps the
// Session's own from the descriptor the Service passes (communication/docs/linux-core-service-contract.md §3, §4)
class SharedMemoryManager::Implementation {
public:
    Implementation(const std::string& memoryName, std::size_t size, bool canWrite);
    Implementation(UniqueFd object, std::size_t size, bool canWrite);
    ~Implementation();

    void *get_memory();

private:
    void map(const UniqueFd &object, bool canWrite);

    void *sharedMemory;
    std::size_t size;
};

SharedMemoryManager::Implementation::Implementation(const std::string& memoryName, std::size_t size, bool canWrite)
        : size(size) {
    UniqueFd object(shm_open(memoryName.c_str(), canWrite ? O_RDWR : O_RDONLY, 0));

    if (object.get() < 0)
        throw contactci::Exception(CCI_ERR_SHM_FAILED_TO_OPEN);

    // Touching a mapping past the end of the object raises SIGBUS and would kill the Client, so refuse a short object
    struct stat info {};

    if (fstat(object.get(), &info) < 0 || static_cast<std::size_t>(info.st_size) < size)
        throw contactci::Exception(CCI_ERR_SHM_FAILED_TO_MAP);

    map(object, canWrite);
}

SharedMemoryManager::Implementation::Implementation(UniqueFd object, std::size_t size, bool canWrite)
        : size(size) {
    // The contract gives every passed object an exact size, and seals it so that it keeps it
    struct stat info {};

    if (object.get() < 0 || fstat(object.get(), &info) < 0 || static_cast<std::size_t>(info.st_size) != size)
        throw contactci::Exception(CCI_ERR_SHM_FAILED_TO_OPEN);

    map(object, canWrite);
}

void SharedMemoryManager::Implementation::map(const UniqueFd &object, bool canWrite) {
    int protection = canWrite ? PROT_READ | PROT_WRITE : PROT_READ;
    sharedMemory = mmap(nullptr, size, protection, MAP_SHARED, object.get(), 0);

    if (sharedMemory == MAP_FAILED)
        throw contactci::Exception(CCI_ERR_SHM_FAILED_TO_MAP);

    // The caller closes the descriptor: the mapping keeps the object alive on its own
}

SharedMemoryManager::Implementation::~Implementation() {
    munmap(sharedMemory, size);
}

void *SharedMemoryManager::Implementation::get_memory() {
    return sharedMemory;
}

SharedMemoryManager::SharedMemoryManager(const std::string& memoryName, std::size_t size, bool canWrite)
    : implementation(std::make_unique<SharedMemoryManager::Implementation>(memoryName, size, canWrite)) {}

SharedMemoryManager::SharedMemoryManager(UniqueFd object, std::size_t size, bool canWrite)
    : implementation(std::make_unique<SharedMemoryManager::Implementation>(std::move(object), size, canWrite)) {}

SharedMemoryManager::~SharedMemoryManager() = default;

void *SharedMemoryManager::get_memory() {
    return implementation->get_memory();
}
