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

protected:
    PipeChannel channel;
};

class HapticSession::Implementation : public Session::Implementation {
public:
    Implementation();
    ~Implementation() = default;
    const HapticStateManager &get_global_haptic_state() const;

protected:
    std::optional<HapticStateManager> global_state_manager;

private:
    void initialize_global_state_manager();
};

class MutableHapticSession::Implementation : public HapticSession::Implementation {
public:
    Implementation();
    ~Implementation() = default;
    MutableHapticStateManager &get_session_haptic_state();

private:
    std::optional<MutableHapticStateManager> session_state_manager;
    void initialize_state_managers();
};

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

Session::Session(std::unique_ptr<Session::Implementation> &&implementation)
    : implementation(std::move(implementation)) {}

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

HapticSession::HapticSession()
    : Session(std::move(std::make_unique<HapticSession::Implementation>())) {}

HapticSession::HapticSession(std::unique_ptr<HapticSession::Implementation> &&implementation)
    : Session(std::move(implementation)) {}

HapticSession::~HapticSession() = default;

const HapticStateManager &HapticSession::get_global_haptic_state() const {
    return static_cast<HapticSession::Implementation*>(implementation.get())->get_global_haptic_state();
}

void HapticSession::Implementation::initialize_global_state_manager() {
    HapticMemoryAccessResponseMessage response = channel.get_haptic_memory_access(true);

    if (!response.wasgranted())
        throw std::runtime_error("Access to haptic memory and events was denied.");

    global_state_manager.emplace(response.readsharedmemoryname(), response.readeventname());
}

HapticSession::Implementation::Implementation() {
    initialize_global_state_manager();
}

const HapticStateManager &HapticSession::Implementation::get_global_haptic_state() const {
    return global_state_manager.value();
}

MutableHapticSession::MutableHapticSession()
    : HapticSession(std::make_unique<MutableHapticSession::Implementation>()) {}

MutableHapticSession::~MutableHapticSession() = default;

void MutableHapticSession::Implementation::initialize_state_managers() {
    HapticMemoryAccessResponseMessage response = channel.get_haptic_memory_access(false);

    if (!response.wasgranted())
        throw std::runtime_error("Access to haptic memory and events was denied.");

    global_state_manager.emplace(response.readsharedmemoryname(), response.readeventname());
    session_state_manager.emplace(response.writesharedmemoryname(), response.writeeventname());
}

MutableHapticStateManager &MutableHapticSession::get_session_haptic_state() {
    return static_cast<MutableHapticSession::Implementation*>(implementation.get())->get_session_haptic_state();
}

MutableHapticSession::Implementation::Implementation() {
    initialize_state_managers();
}

MutableHapticStateManager &MutableHapticSession::Implementation::get_session_haptic_state() {
    return session_state_manager.value();
}
