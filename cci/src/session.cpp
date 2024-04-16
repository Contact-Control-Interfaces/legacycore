//
// Created by john_contactci on 3/18/2024.
//

#include <optional>
#include "session.h"

using namespace contactci;

contactci::ClientDescription::ClientDescription(uint32_t process_id, std::string process_name)
    : process_id(process_id), process_name(std::move(process_name)) {}

uint32_t contactci::ClientDescription::get_process_id() const {
    return process_id;
}

const std::string &contactci::ClientDescription::get_process_name() const {
    return process_name;
}

contactci::DeviceDescription::DeviceDescription(
    std::string product_line, std::string serial_number, bool is_right, bool is_connected
) : product_line(std::move(product_line)), serial_number(std::move(serial_number)),
    is_right(is_right), is_connected(is_connected) {}

const std::string &contactci::DeviceDescription::get_product_line() const {
    return product_line;
}

const std::string &contactci::DeviceDescription::get_serial_number() const {
    return serial_number;
}

bool contactci::DeviceDescription::get_is_right() const {
    return is_right;
}

bool contactci::DeviceDescription::get_is_connected() const {
    return is_connected;
}

contactci::ServiceInfo::ServiceInfo(std::string version, bool is_interactive)
    : version(version), is_interactive(is_interactive) {}

const std::string &contactci::ServiceInfo::get_version() const {
    return version;
}

bool contactci::ServiceInfo::get_is_interactive() const {
    return is_interactive;
}

class Session::Implementation {
public:
    Implementation() = default;
    ~Implementation() = default;

    std::vector<contactci::DeviceDescription> get_device_list();
    std::vector<contactci::ClientDescription> get_client_list();
    contactci::ServiceInfo get_service_info();
    HapticMemoryAccessResponseMessage get_haptic_memory_access(bool observeOnly);

private:
    PipeChannel channel;
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
    ~Implementation() = default;

    MutableHapticStateManager &get_session_haptic_state();

private:
    MutableHapticStateManager session_state_manager;
};

HapticMemoryAccessResponseMessage Session::Implementation::get_haptic_memory_access(bool observeOnly) {
    return channel.get_haptic_memory_access(observeOnly);
}

std::vector<contactci::DeviceDescription> Session::Implementation::get_device_list() {
    DeviceListResponseMessage response = channel.get_device_list();
    std::vector<contactci::DeviceDescription> result;

    std::transform(
        std::begin(response.devices()), std::end(response.devices()),
        std::back_inserter(result),
        [](const DeviceDescriptionMessage& msg) {
            return contactci::DeviceDescription(
                msg.productline(), msg.serialnumber(), msg.isright(), msg.isconnected()
            );
        }
    );

    return result;
}

std::vector<contactci::ClientDescription> Session::Implementation::get_client_list() {
    ClientListResponseMessage response = channel.get_client_list();
    std::vector<contactci::ClientDescription> result;

    std::transform(
        std::begin(response.clients()), std::end(response.clients()),
        std::back_inserter(result),
        [](const ClientDescriptionMessage& msg) {
            return contactci::ClientDescription(msg.processid(), msg.processname());
        }
    );

    return result;
}

contactci::ServiceInfo Session::Implementation::get_service_info() {
    ServiceInfoResponseMessage response = channel.get_service_info();

    return {response.version(), response.isinteractive()};
}

Session::Session()
    : implementation(std::make_unique<Session::Implementation>()) {}

Session::~Session() = default;

std::vector<contactci::DeviceDescription> Session::get_device_list() {
    return implementation->get_device_list();
}

std::vector<contactci::ClientDescription> Session::get_client_list() {
    return implementation->get_client_list();
}

contactci::ServiceInfo Session::get_service_info() {
    return implementation->get_service_info();
}

HapticSession::HapticSession(std::unique_ptr<HapticSession::Implementation> &&implementation)
    : implementation(std::move(implementation)) { }

HapticSession::HapticSession() {
    HapticMemoryAccessResponseMessage response = Session::implementation->get_haptic_memory_access(true);

    if (!response.wasgranted())
        throw std::runtime_error("Access to haptic memory and events was denied.");

    HapticSession::implementation = std::make_unique<HapticSession::Implementation>(response.readsharedmemoryname(), response.readeventname());
}

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
    HapticMemoryAccessResponseMessage response = Session::implementation->get_haptic_memory_access(false);

    if (!response.wasgranted())
        throw std::runtime_error("Access to haptic memory and events was denied.");

    HapticSession::implementation = std::make_unique<HapticSession::Implementation>(response.readsharedmemoryname(), response.readeventname());
    MutableHapticSession::implementation = std::make_unique<MutableHapticSession::Implementation>(response.writesharedmemoryname(), response.writeeventname());
}

MutableHapticSession::~MutableHapticSession() = default;

MutableHapticStateManager &MutableHapticSession::get_session_haptic_state() {
    return MutableHapticSession::implementation->get_session_haptic_state();
}

MutableHapticSession::Implementation::Implementation(
    const std::string &sessionMemoryName,const std::string &sessionEventName
) : session_state_manager(sessionMemoryName, sessionEventName) {}

MutableHapticStateManager &MutableHapticSession::Implementation::get_session_haptic_state() {
    return session_state_manager;
}
