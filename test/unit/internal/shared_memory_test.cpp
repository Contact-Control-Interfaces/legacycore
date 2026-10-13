#include <gtest/gtest.h>

#include "cci/error.h"
#include "cci/shared_memory.h"
#include "win_objects.h"

using testsupport::TestSharedMemory;

namespace {
    CciStatus status_of_opening(const std::string &name, std::size_t size) {
        try {
            contactci::SharedMemoryManager memory(name, size, false);
        } catch (const contactci::Exception &e) {
            return e.get_error_code();
        }
        return CCI_SUCCESS;
    }
}

TEST(SharedMemory, OpeningAMissingSectionThrowsFailedToOpen) {
    EXPECT_EQ(status_of_opening(testsupport::unique_name("missing"), 2 * sizeof(HapticState)),
              CCI_ERR_SHM_FAILED_TO_OPEN);
}

TEST(SharedMemory, MappingLargerThanTheSectionThrowsFailedToMap) {
    TestSharedMemory serviceSide;
    EXPECT_EQ(status_of_opening(serviceSide.name(), 1024 * 1024), CCI_ERR_SHM_FAILED_TO_MAP);
}

TEST(SharedMemory, ReadsWhatTheServiceWrites) {
    TestSharedMemory serviceSide;
    serviceSide.left()->indexVibrationAmplitude = 0.75f;

    contactci::SharedMemoryManager memory(serviceSide.name(), 2 * sizeof(HapticState), false);

    EXPECT_FLOAT_EQ(static_cast<HapticState *>(memory.get_memory())->indexVibrationAmplitude, 0.75f);
}

TEST(SharedMemory, WritableMappingWritesThrough) {
    TestSharedMemory serviceSide;
    contactci::SharedMemoryManager memory(serviceSide.name(), 2 * sizeof(HapticState), true);

    static_cast<HapticState *>(memory.get_memory())[1].thumbForceFeedbackAmplitude = 0.5f;

    EXPECT_FLOAT_EQ(serviceSide.right()->thumbForceFeedbackAmplitude, 0.5f);
}
