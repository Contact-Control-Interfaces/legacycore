//
// Created by john_contactci on 4/18/2024.
//

#pragma once

#include <string>
#include <optional>
#include <thread>
#include <atomic>
#include <mutex>

#include "pipe_channel.h"
#include "named_event.h"
#include "session.h"

namespace contactci {
    template<typename T>
    class EventDrivenValueMonitor {
    public:
        EventDrivenValueMonitor(PipeChannel &channel, const std::string &eventName);
        virtual ~EventDrivenValueMonitor();

        T get_value();

    protected:
        PipeChannel &channel;

        void check_and_rethrow() const;

    private:
        virtual T get_new_value() = 0;
        void update_value();

        NamedEvent event;
        T value;
        std::atomic<bool> isRunning;
        std::mutex lock;
        std::thread thread;
        std::exception_ptr threadException;
    };

    template<typename T>
    EventDrivenValueMonitor<T>::EventDrivenValueMonitor(contactci::PipeChannel &channel, const std::string &eventName)
        : channel(channel), event(eventName, false), isRunning(true),
          thread(&EventDrivenValueMonitor::update_value, this) {}

    template<typename T>
    EventDrivenValueMonitor<T>::~EventDrivenValueMonitor() {
        isRunning = false;
        thread.join();
    }

    template<typename T>
    void EventDrivenValueMonitor<T>::check_and_rethrow() const {
        if (!isRunning.load())
            std::rethrow_exception(threadException);
    }

    template<typename T>
    T EventDrivenValueMonitor<T>::get_value() {
        check_and_rethrow();

        lock.lock();
        T result = value;
        lock.unlock();
        return result;
    }

    template<typename T>
    void EventDrivenValueMonitor<T>::update_value() {
        try {
            if (!isRunning.load())
                return;

            lock.lock();
            value = get_new_value();
            lock.unlock();

            while (isRunning.load()) {
                if (!event.wait(100))
                    continue;

                if (!isRunning.load())
                    return;

                lock.lock();
                value = get_new_value();
                lock.unlock();
            }
        } catch (...) {
            threadException = std::current_exception();
            isRunning.store(false);
        }
    }

    class DeviceMonitor : public EventDrivenValueMonitor<std::vector<contactci::DeviceDescription>> {
    public:
        DeviceMonitor(PipeChannel &channel, const std::string &eventName);
        ~DeviceMonitor() = default;

        std::optional<contactci::DeviceDescription> get_left_device();
        std::optional<contactci::DeviceDescription> get_right_device();

    private:
        std::vector<contactci::DeviceDescription> get_new_value();

        std::optional<contactci::DeviceDescription> leftDevice;
        std::optional<contactci::DeviceDescription> rightDevice;
    };

    class ClientMonitor : public EventDrivenValueMonitor<std::vector<contactci::ClientDescription>> {
    public:
        ClientMonitor(PipeChannel &channel, const std::string &eventName);
        ~ClientMonitor() = default;

    private:
        std::vector<contactci::ClientDescription> get_new_value();
    };
}