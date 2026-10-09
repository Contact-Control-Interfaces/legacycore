#include <gtest/gtest.h>

#include "cci/named_event.h"
#include "win_objects.h"

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

TEST(NamedEvent, WritableEventSetsAndResets) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), true);

    event.set();
    EXPECT_TRUE(serviceSide.is_set());
    event.reset();
    EXPECT_FALSE(serviceSide.is_set());
}

// A read-only open asks only for SYNCHRONIZE, so Windows refuses to set it.
TEST(NamedEvent, ReadOnlyEventCannotBeSet) {
    TestEvent serviceSide;
    contactci::NamedEvent event(serviceSide.name(), false);

    try {
        event.set();
        FAIL() << "set() on a read-only event should throw";
    } catch (const contactci::Exception &e) {
        EXPECT_EQ(e.get_error_code(), CCI_ERR_EVENT_FAILED_TO_SET);
    }
    EXPECT_FALSE(serviceSide.is_set());
}
