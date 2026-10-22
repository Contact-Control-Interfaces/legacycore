#include <gtest/gtest.h>

#include "cci/named_event.h"
#include "test_objects.h"

using testsupport::TestEvent;

namespace {
    CciStatus status_of(void (*action)(const std::string &), const std::string &name) {
        try {
            action(name);
        } catch (const contactci::Exception &e) {
            return e.get_error_code();
        }
        return CCI_SUCCESS;
    }
}

TEST(NamedEvent, OpeningAMissingEventThrowsFailedToOpen) {
    auto open = [](const std::string &name) { contactci::NamedEvent event(name, false); };
    EXPECT_EQ(status_of(open, testsupport::unique_name("missing")), CCI_ERR_EVENT_FAILED_TO_OPEN);
}

TEST(NamedEvent, WaitReportsTheEventsState) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), false);

    EXPECT_FALSE(event.wait(10));
    serviceSide.set();
    EXPECT_TRUE(event.wait(10));
}

#ifdef _WIN32
// Windows only: on Linux the signal is a counter, and reset() does nothing
// (named_event_posix_test.cpp covers it).
TEST(NamedEvent, WritableEventSetsAndResets) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), true);

    event.set();
    EXPECT_TRUE(serviceSide.is_set());
    event.reset();
    EXPECT_FALSE(serviceSide.is_set());
}
#endif

// A read-only open asks only for SYNCHRONIZE, so Windows refuses to set it.
// On Linux the counter is mapped read-only, and Core refuses before writing.
TEST(NamedEvent, ReadOnlyEventCannotBeSet) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), false);

    try {
        event.set();
        FAIL() << "set() on a read-only event should throw";
    } catch (const contactci::Exception &e) {
        EXPECT_EQ(e.get_error_code(), CCI_ERR_EVENT_FAILED_TO_SET);
    }
#ifdef _WIN32
    EXPECT_FALSE(serviceSide.is_set());
#else
    EXPECT_EQ(serviceSide.count(), 0u);
#endif
}
