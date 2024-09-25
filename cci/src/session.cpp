//
// Created by john_contactci on 3/18/2024.
//

#include "cci/session.h"
#include "cci/value_monitor.h"

using namespace contactci;

class Session::Implementation {
public:
    Implementation(
        PipeChannel &&channel,
        const std::string &clientsChangedEventName, const std::string &devicesChangedEventName,
        std::string serviceVersion, bool isServiceInteractive
    );
    ~Implementation() = default;

    bool is_connected();

    std::vector<contactci::DeviceDescription> get_device_list();
    std::vector<contactci::ClientDescription> get_client_list();

    std::optional<contactci::DeviceDescription> &get_left_device();
    std::optional<contactci::DeviceDescription> &get_right_device();

    const std::string &get_service_version() const;
    bool is_service_interactive() const;

    const DeviceManager &get_device_manager() const;

private:
    PipeChannel channel;

    ClientMonitor clientMonitor;
    DeviceMonitor deviceMonitor;
    DeviceManager deviceManager;

    // We need to store these here so that the memory for the strings in `DeviceDescription` is part of the session
    // for the C API. Accessing the optionals in `DeviceMonitor` requires locking to ensure they aren't in the middle
    // of being overwritten, so instead we just stored the last-read devices here.
    std::optional<contactci::DeviceDescription> leftDevice;
    std::optional<contactci::DeviceDescription> rightDevice;

    std::string serviceVersion;
    bool isServiceInteractive;
};

class HapticSession::Implementation {
public:
    Implementation(const std::string& globalMemoryName, const std::string& globalEventName, const DeviceManager &device_manager);
    ~Implementation() = default;

    const HapticStateManager &get_global_haptic_state() const;
    const HapticDeviceManager &get_global_device_manager() const;

private:
    HapticStateManager global_state_manager;
    HapticDeviceManager global_device_manager;
};

class MutableHapticSession::Implementation {
public:
    Implementation(const std::string& sessionMemoryName, const std::string& sessionEventName, const DeviceManager &device_manager);
    ~Implementation() = default;

    MutableHapticStateManager &get_session_haptic_state();
    MutableHapticDeviceManager &get_session_device_manager();

private:
    MutableHapticStateManager session_state_manager;
    MutableHapticDeviceManager session_device_manager;
};

Session::Implementation::Implementation(
    PipeChannel &&channel,
    const std::string &clientsChangedEventName, const std::string &devicesChangedEventName,
    std::string serviceVersion, bool isServiceInteractive
) : channel(std::move(channel)),
    clientMonitor(this->channel, clientsChangedEventName),
    deviceMonitor(this->channel, devicesChangedEventName),
    deviceManager(this->channel),
    leftDevice(std::nullopt), rightDevice(std::nullopt),
    serviceVersion(std::move(serviceVersion)),
    isServiceInteractive(isServiceInteractive) {}

bool Session::Implementation::is_connected() {
    return channel.is_connected();
}

std::vector<contactci::DeviceDescription> Session::Implementation::get_device_list() {
    return deviceMonitor.get_value();
}

std::vector<contactci::ClientDescription> Session::Implementation::get_client_list() {
    return clientMonitor.get_value();
}

std::optional<contactci::DeviceDescription> &Session::Implementation::get_left_device() {
    leftDevice = deviceMonitor.get_left_device();
    return leftDevice;
}

std::optional<contactci::DeviceDescription> &Session::Implementation::get_right_device() {
    rightDevice = deviceMonitor.get_right_device();
    return rightDevice;
}

const std::string &Session::Implementation::get_service_version() const {
    return serviceVersion;
}

bool Session::Implementation::is_service_interactive() const {
    return isServiceInteractive;
}

const DeviceManager & Session::Implementation::get_device_manager() const {
    return deviceManager;
}

const HapticDeviceManager & HapticSession::Implementation::get_global_device_manager() const {
    return global_device_manager;
}

Session::Session() {
    PipeChannel channel;
    SessionInitializationResponseMessage response = channel.initialize_session(false, false);

    if (!response.ishapticaccessgranted())
        throw contactci::Exception(CCI_ERR_SESSION_ACCESS_DENIED);

    Session::implementation = std::make_unique<Session::Implementation>(
        std::move(channel),
        response.clientschangedeventname(), response.deviceschangedeventname(),
        response.version(), response.isinteractive()
    );
}

Session::Session(std::unique_ptr<Session::Implementation> &&implementation)
    : implementation(std::move(implementation)) {}

Session::~Session() = default;

bool Session::is_connected() {
    return implementation->is_connected();
}

std::vector<contactci::DeviceDescription> Session::get_device_list() {
    return implementation->get_device_list();
}

std::vector<contactci::ClientDescription> Session::get_client_list() {
    return implementation->get_client_list();
}

const std::optional<contactci::DeviceDescription> &Session::get_left_device() const {
    return implementation->get_left_device();
}

const std::optional<contactci::DeviceDescription> &Session::get_right_device() const {
    return implementation->get_right_device();
}

const std::string &Session::get_service_version() const {
    return implementation->get_service_version();
}

bool Session::is_service_interactive() const {
    return implementation->is_service_interactive();
}

const DeviceManager & Session::get_device_manager() const {
    return implementation->get_device_manager();
}

HapticSession::HapticSession() : Session(nullptr) {
    PipeChannel channel;
    SessionInitializationResponseMessage response = channel.initialize_session(true, false);

    if (!response.ishapticaccessgranted())
        throw contactci::Exception(CCI_ERR_SESSION_ACCESS_DENIED);

    Session::implementation = std::make_unique<Session::Implementation>(
        std::move(channel),
        response.clientschangedeventname(), response.deviceschangedeventname(),
        response.version(), response.isinteractive()
    );
    HapticSession::implementation = std::make_unique<HapticSession::Implementation>(
        response.hapticreadsharedmemoryname(), response.hapticreadeventname(), Session::implementation->get_device_manager()
    );
}

HapticSession::HapticSession(std::unique_ptr<HapticSession::Implementation> &&implementation)
    : Session(nullptr), implementation(std::move(implementation)) {}

HapticSession::~HapticSession() = default;

const HapticStateManager &HapticSession::get_global_haptic_state() const {
    return HapticSession::implementation->get_global_haptic_state();
}

HapticSession::Implementation::Implementation(const std::string& globalMemoryName, const std::string& globalEventName, const DeviceManager &device_manager)
    : global_state_manager(globalMemoryName, globalEventName), global_device_manager(device_manager) {}

const HapticStateManager &HapticSession::Implementation::get_global_haptic_state() const {
    return global_state_manager;
}

const HapticDeviceManager & HapticSession::get_global_device_manager() const {
    return implementation->get_global_device_manager();
}

MutableHapticSession::MutableHapticSession() : HapticSession(nullptr) {
    PipeChannel channel;
    SessionInitializationResponseMessage response = channel.initialize_session(true, true);

    if (!response.ishapticaccessgranted())
        throw contactci::Exception(CCI_ERR_SESSION_ACCESS_DENIED);

    Session::implementation = std::make_unique<Session::Implementation>(
        std::move(channel),
        response.clientschangedeventname(), response.deviceschangedeventname(),
        response.version(), response.isinteractive()
    );
    HapticSession::implementation = std::make_unique<HapticSession::Implementation>(
        response.hapticreadsharedmemoryname(), response.hapticreadeventname(), Session::implementation->get_device_manager()
    );
    MutableHapticSession::implementation = std::make_unique<MutableHapticSession::Implementation>(
        response.hapticwritesharedmemoryname(), response.hapticwriteeventname(), Session::implementation->get_device_manager()
    );
}

MutableHapticSession::~MutableHapticSession() = default;

MutableHapticStateManager &MutableHapticSession::get_session_haptic_state() {
    return MutableHapticSession::implementation->get_session_haptic_state();
}

MutableHapticDeviceManager & MutableHapticSession::get_session_device_manager() {
    return implementation->get_session_device_manager();
}

MutableHapticSession::Implementation::Implementation(
    const std::string &sessionMemoryName, const std::string &sessionEventName, const DeviceManager &device_manager
) : session_state_manager(sessionMemoryName, sessionEventName), session_device_manager(device_manager) {}

MutableHapticStateManager &MutableHapticSession::Implementation::get_session_haptic_state() {
    return session_state_manager;
}

MutableHapticDeviceManager & MutableHapticSession::Implementation::get_session_device_manager() {
    return session_device_manager;
}