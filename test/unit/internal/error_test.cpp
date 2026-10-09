#include <cstring>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

#include "cci/error.h"

TEST(ErrorStrings, EveryStatusHasAMessage) {
    for (int status = CCI_SUCCESS; status <= CCI_ERR_SHM_FAILED_TO_MAP; ++status) {
        const char *message = get_error_string(static_cast<CciStatus>(status));
        ASSERT_NE(message, nullptr) << "status " << status;
        EXPECT_GT(std::strlen(message), 0u) << "status " << status;
    }
}

TEST(ErrorStrings, MessagesMatchTheirStatus) {
    EXPECT_STREQ(get_error_string(CCI_SUCCESS), "Success");
    EXPECT_STREQ(get_error_string(CCI_NO_DEVICE), "No device exists");
    EXPECT_STREQ(get_error_string(CCI_ERR_SESSION_ACCESS_DENIED),
                 "Session initialization failed due to service rejecting requested access");
    EXPECT_STREQ(get_error_string(CCI_ERR_PIPE_FAILED_TO_OPEN), "Failed to open pipe");
    EXPECT_STREQ(get_error_string(CCI_ERR_SHM_FAILED_TO_MAP), "Failed to map view of shared memory");
}

TEST(ErrorStrings, OutOfRangeStatusHasNoMessage) {
    EXPECT_EQ(get_error_string(static_cast<CciStatus>(CCI_ERR_SHM_FAILED_TO_MAP + 1)), nullptr);
    EXPECT_EQ(get_error_string(static_cast<CciStatus>(-1)), nullptr);
}

TEST(Exception, CarriesItsStatus) {
    try {
        throw contactci::Exception(CCI_ERR_PIPE_FAILED_TO_READ);
    } catch (const contactci::Exception &e) {
        EXPECT_EQ(e.get_error_code(), CCI_ERR_PIPE_FAILED_TO_READ);
    }
}

// Known defect, pinned so it is visible in every test report. Exception is
// declared `class Exception : std::runtime_error`, and a class's bases are
// private by default - so `catch (const std::exception &)` never catches it and
// what() is inaccessible to callers. Fixing it (`: public std::runtime_error`)
// makes this test fail on purpose: update it to assert the fixed behaviour.
TEST(Exception, KnownDefect_NotCatchableAsStdException) {
    EXPECT_FALSE((std::is_convertible_v<contactci::Exception *, std::exception *>));

    bool caughtAsStdException = false;
    try {
        try {
            throw contactci::Exception(CCI_UNKNOWN_ERROR);
        } catch (const std::exception &) {
            caughtAsStdException = true;
        }
    } catch (const contactci::Exception &) {
    }
    EXPECT_FALSE(caughtAsStdException);
}
