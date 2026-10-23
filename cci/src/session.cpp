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

private:
    PipeChannel channel;

    ClientMonitor clientMonitor;
    DeviceMonitor deviceMonitor;

    std::optional<contactci::DeviceDescription> leftDevice;
    std::optional<contactci::DeviceDescription> rightDevice;

    std::string serviceVersion;
    bool isServiceInteractive;
};

class HapticSession::Implementation {
public:
    Implementation(const std::string& globalMemoryName, const std::string& globalEventName);
    ~Implementation() = default;

    const HapticStateManager &get_global_haptic_state() const;

private:
    HapticStateManager global_state_manager;
};

class MutableHapticSession::Implementation {
public:
    Implementation(const std::string& sessionMemoryName, const std::string& sessionEventName);
#ifndef _WIN32
    Implementation(UniqueFd sessionRegion, UniqueFd sessionCounter);
#endif
    ~Implementation() = default;

    MutableHapticStateManager &get_session_haptic_state();

private:
    MutableHapticStateManager session_state_manager;
};

Session::Implementation::Implementation(
    PipeChannel &&channel,
    const std::string &clientsChangedEventName, const std::string &devicesChangedEventName,
    std::string serviceVersion, bool isServiceInteractive
) : channel(std::move(channel)),
    clientMonitor(this->channel, clientsChangedEventName),
    deviceMonitor(this->channel, devicesChangedEventName),
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

Session::Session() {
    PipeChannel channel;
    SessionInitializationResponseMessage response = channel.initialize_session(false, false);

    if (!response.ishapticaccessgranted())
        throw contactci::Exception(CCI_ERR_SESSION_ACCESS_DENIED);

#ifndef _WIN32
    // Only a write Session's reply carries descriptors (communication's docs/linux-core-service-contract.md §3)
    channel.take_session_descriptors(0);
#endif

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

HapticSession::HapticSession() : Session(nullptr) {
    PipeChannel channel;
    SessionInitializationResponseMessage response = channel.initialize_session(true, false);

    if (!response.ishapticaccessgranted())
        throw contactci::Exception(CCI_ERR_SESSION_ACCESS_DENIED);

#ifndef _WIN32
    // Only a write Session's reply carries descriptors (contract §3)
    channel.take_session_descriptors(0);
#endif

    Session::implementation = std::make_unique<Session::Implementation>(
        std::move(channel),
        response.clientschangedeventname(), response.deviceschangedeventname(),
        response.version(), response.isinteractive()
    );
    HapticSession::implementation = std::make_unique<HapticSession::Implementation>(
        response.hapticreadsharedmemoryname(), response.hapticreadeventname()
    );
}

HapticSession::HapticSession(std::unique_ptr<HapticSession::Implementation> &&implementation)
    : Session(nullptr), implementation(std::move(implementation)) {}

HapticSession::~HapticSession() = default;

const HapticStateManager &HapticSession::get_global_haptic_state() const {
    return HapticSession::implementation->get_global_haptic_state();
}

HapticSession::Implementation::Implementation(const std::string& globalMemoryName, const std::string& globalEventName)
    : global_state_manager(globalMemoryName, globalEventName) {}

const HapticStateManager &HapticSession::Implementation::get_global_haptic_state() const {
    return global_state_manager;
}

MutableHapticSession::MutableHapticSession() : HapticSession(nullptr) {
    PipeChannel channel;
    SessionInitializationResponseMessage response = channel.initialize_session(true, true);

    if (!response.ishapticaccessgranted())
        throw contactci::Exception(CCI_ERR_SESSION_ACCESS_DENIED);

    // This Session's own objects first, so a bad one fails the Session before its monitor threads start
#ifdef _WIN32
    auto sessionState = std::make_unique<MutableHapticSession::Implementation>(
        response.hapticwritesharedmemoryname(), response.hapticwriteeventname()
    );
#else
    // Linux passes them as descriptors [0] and [1] on the reply, and the two name fields only hold the placeholders
    // fd:0 and fd:1 (contract §3). Nobody else can open them: they have no name
    std::vector<UniqueFd> descriptors = channel.take_session_descriptors(2);

    // Real names here, next to the descriptors, mean a Service that still creates named write objects too, which any
    // contactci member could open (DEV-106)
    if (response.hapticwritesharedmemoryname() != "fd:0" || response.hapticwriteeventname() != "fd:1")
        throw contactci::Exception(CCI_ERR_SHM_FAILED_TO_OPEN);
    auto sessionState = std::make_unique<MutableHapticSession::Implementation>(
        std::move(descriptors[0]), std::move(descriptors[1])
    );
#endif

    Session::implementation = std::make_unique<Session::Implementation>(
        std::move(channel),
        response.clientschangedeventname(), response.deviceschangedeventname(),
        response.version(), response.isinteractive()
    );
    HapticSession::implementation = std::make_unique<HapticSession::Implementation>(
        response.hapticreadsharedmemoryname(), response.hapticreadeventname()
    );
    MutableHapticSession::implementation = std::move(sessionState);
}

MutableHapticSession::~MutableHapticSession() = default;

MutableHapticStateManager &MutableHapticSession::get_session_haptic_state() {
    return MutableHapticSession::implementation->get_session_haptic_state();
}

MutableHapticSession::Implementation::Implementation(
    const std::string &sessionMemoryName, const std::string &sessionEventName
) : session_state_manager(sessionMemoryName, sessionEventName) {}

#ifndef _WIN32
MutableHapticSession::Implementation::Implementation(UniqueFd sessionRegion, UniqueFd sessionCounter)
    : session_state_manager(std::move(sessionRegion), std::move(sessionCounter)) {}
#endif

MutableHapticStateManager &MutableHapticSession::Implementation::get_session_haptic_state() {
    return session_state_manager;
}
