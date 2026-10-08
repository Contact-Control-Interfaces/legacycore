//
// Created by john_contactci on 4/18/2024.
//

#pragma once

#include <string>
#include <optional>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>

#include "pipe_channel.h"
#include "named_event.h"
#include "session.h"

namespace contactci {
    // Fetches a value through the channel, then refetches it whenever the
    // service signals `eventName`, on a thread of its own.
    //
    // The thread calls the virtual get_new_value(), so it must only run while
    // the derived object is fully alive: derived classes call start() as the
    // last thing in their constructor and stop() as the first thing in their
    // destructor. Starting it from this base constructor (as it used to) let it
    // run derived code before the derived object existed, and joining it here
    // let it run after the derived object was destroyed - a crash on session
    // open or close.
    template<typename T>
    class EventDrivenValueMonitor {
    public:
        EventDrivenValueMonitor(PipeChannel &channel, const std::string &eventName);
        virtual ~EventDrivenValueMonitor();

        T get_value();

    protected:
        PipeChannel &channel;

        void start();
        void stop();
        void check_and_rethrow() const;

    private:
        virtual T get_new_value() = 0;
        void update_value();
        void monitor_value();

        NamedEvent event;
        T value;
        std::atomic<bool> isRunning;
        std::mutex lock;
        std::thread thread;
        std::exception_ptr threadException;
        std::promise<void> valuePromise;
        std::future<void> valueFuture;
    };

    template<typename T>
    EventDrivenValueMonitor<T>::EventDrivenValueMonitor(contactci::PipeChannel &channel, const std::string &eventName)
            : channel(channel), event(eventName, false), isRunning(false) {
        valueFuture = valuePromise.get_future();
    }

    template<typename T>
    EventDrivenValueMonitor<T>::~EventDrivenValueMonitor() {
        stop();   // normally a no-op: the derived destructor has already stopped the thread
    }

    template<typename T>
    void EventDrivenValueMonitor<T>::start() {
        isRunning = true;
        thread = std::thread(&EventDrivenValueMonitor::monitor_value, this);
    }

    template<typename T>
    void EventDrivenValueMonitor<T>::stop() {
        isRunning = false;
        if (thread.joinable())
            thread.join();
    }

    template<typename T>
    void EventDrivenValueMonitor<T>::check_and_rethrow() const {
        // Not running after a failure carries the thread's exception; not
        // running because stop() was called carries none.
        if (!isRunning.load() && threadException)
            std::rethrow_exception(threadException);
    }

    template<typename T>
    T EventDrivenValueMonitor<T>::get_value() {
        check_and_rethrow();

        valueFuture.wait(); // Used to ensure the thread has fetched an initial value

        // The promise is also completed when the first fetch fails, so a caller
        // that was already waiting gets that error here instead of hanging.
        check_and_rethrow();

        std::lock_guard<std::mutex> guard(lock);
        return value;
    }

    template<typename T>
    void EventDrivenValueMonitor<T>::update_value() {
        // Fetch outside the lock: a failed fetch must not leave it held, and
        // readers should not wait on pipe I/O.
        T fresh = get_new_value();
        std::lock_guard<std::mutex> guard(lock);
        value = std::move(fresh);
    }

    template<typename T>
    void EventDrivenValueMonitor<T>::monitor_value() {
        bool initialValueFetched = false;
        try {
            if (!isRunning.load())
                return;

            update_value();

            initialValueFetched = true;
            valuePromise.set_value(); // Indicate that we've fetched an initial value

            while (isRunning.load()) {
                if (!event.wait(100))
                    continue;

                if (!isRunning.load())
                    return;

                update_value();
            }
        } catch (...) {
            threadException = std::current_exception();
            isRunning.store(false);
            // Release anyone waiting for the first value; get_value() rethrows the error.
            if (!initialValueFetched)
                valuePromise.set_value();
        }
    }

    class DeviceMonitor : public EventDrivenValueMonitor<std::vector<contactci::DeviceDescription>> {
    public:
        DeviceMonitor(PipeChannel &channel, const std::string &eventName);
        ~DeviceMonitor();

        std::optional<contactci::DeviceDescription> get_left_device();
        std::optional<contactci::DeviceDescription> get_right_device();

    private:
        std::vector<contactci::DeviceDescription> get_new_value();
        std::optional<contactci::DeviceDescription> connected_device(bool isRight);
    };

    class ClientMonitor : public EventDrivenValueMonitor<std::vector<contactci::ClientDescription>> {
    public:
        ClientMonitor(PipeChannel &channel, const std::string &eventName);
        ~ClientMonitor();

    private:
        std::vector<contactci::ClientDescription> get_new_value();
    };
}