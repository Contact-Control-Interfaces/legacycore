//
// Created by zach_contactci on 10/13/2026.
//

#include "cci/shared_memory.h"
#include "cci/error.h"

using namespace contactci;

// TODO(DEV-7): shm_open the region created by the Service (O_RDWR or O_RDONLY per canWrite), mmap, munmap on destruction
class SharedMemoryManager::Implementation {
public:
    Implementation(const std::string& memoryName, std::size_t size, bool canWrite) {
        throw contactci::Exception(CCI_ERR_SHM_FAILED_TO_OPEN);
    }

    void *get_memory() {
        return nullptr;
    }
};

SharedMemoryManager::SharedMemoryManager(const std::string& memoryName, std::size_t size, bool canWrite)
    : implementation(std::make_unique<SharedMemoryManager::Implementation>(memoryName, size, canWrite)) {}

SharedMemoryManager::~SharedMemoryManager() = default;

void *SharedMemoryManager::get_memory() {
    return implementation->get_memory();
}
