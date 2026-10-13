#pragma once

#include <chrono>
#include <memory>
#include <thread>

#include <gtest/gtest.h>

#include "fake_service.h"

// A fresh fake service for every test. Sessions are created inside the test
// body, so they are always closed before the service goes away.
class WithFakeService : public ::testing::Test {
protected:
    void SetUp() override {
        if (testsupport::FakeService::real_service_running())
            GTEST_SKIP() << "A Contact CI service owns " << testsupport::FakeService::PipeName
                         << " on this machine; stop it to run the API tests.";
        service = std::make_unique<testsupport::FakeService>();
    }

    void TearDown() override { service.reset(); }

    // Polls until `condition` holds; the library refreshes lists on its own threads.
    template <typename Condition>
    static bool eventually(Condition condition, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (condition()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return condition();
    }

    std::unique_ptr<testsupport::FakeService> service;
};

// No service at all: the pipe does not exist.
class WithoutService : public ::testing::Test {
protected:
    void SetUp() override {
        if (testsupport::FakeService::real_service_running())
            GTEST_SKIP() << "A Contact CI service is running on this machine.";
    }
};
