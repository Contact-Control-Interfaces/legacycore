#pragma once

#include <chrono>
#include <memory>
#include <thread>

#include <gtest/gtest.h>

#ifdef _WIN32
#include "fake_service.h"
#else
#include "fake_daemon_posix.h"
#endif

// A fresh fake service for every test. Sessions are created inside the test
// body, so they are always closed before the service goes away.
//
// On Linux the fake is the fake daemon (fake_daemon_posix.h). It listens on a
// socket of its own and points Core at it with CONTACTCI_SERVICE_SOCKET, so a
// real Service on the same machine is never in the way.
class WithFakeService : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        if (testsupport::FakeService::real_service_running())
            GTEST_SKIP() << "A Contact CI service owns " << testsupport::FakeService::PipeName
                         << " on this machine; stop it to run the API tests.";
#endif
        service = std::make_unique<testsupport::FakeService>();
    }

    void TearDown() override {
#ifndef _WIN32
        // Whatever the test checked, Core must have kept to the contract on the wire.
        if (service)
            for (const auto &error : service->protocol_errors())
                ADD_FAILURE() << "Core broke the Linux Core-Service contract: " << error;
#endif
        service.reset();
    }

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
#ifdef _WIN32
    void SetUp() override {
        if (testsupport::FakeService::real_service_running())
            GTEST_SKIP() << "A Contact CI service is running on this machine.";
    }
#else
    // Core is pointed at a socket path that doesn't exist, so a real Service isn't reached either.
    void SetUp() override {
        noService = std::make_unique<testsupport::ScopedEnvironment>(testsupport::ServiceSocketVariable,
                                                                     testsupport::missing_socket_path());
    }

    void TearDown() override { noService.reset(); }

    std::unique_ptr<testsupport::ScopedEnvironment> noService;
#endif
};
