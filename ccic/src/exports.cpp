//
// Created by john_contactci on 3/9/2022.
//
#include <algorithm>
#include <optional>

#include "contactci.h"
#include <session.h>

using namespace contactci;

struct DeviceListingTransaction {
    std::vector<contactci::DeviceDescription> devices;
};

struct ClientListingTransaction {
    std::vector<contactci::ClientDescription> clients;
};

struct ServiceInfoTransaction {
    std::optional<contactci::ServiceInfo> info;
};

DeviceListingTransactionHandle cci_start_device_listing_transaction() {
    return new DeviceListingTransaction;
}

size_t cci_fetch_device_listing(CciSessionHandle sessionHandle, DeviceListingTransactionHandle transactionHandle) {
    auto* session = reinterpret_cast<Session*>(sessionHandle);
    auto *transaction = reinterpret_cast<DeviceListingTransaction*>(transactionHandle);

    transaction->devices = session->get_device_list();

    return transaction->devices.size();
}

void cci_get_device_listing(DeviceListingTransactionHandle transactionHandle, ::DeviceDescription *out) {
    auto *transaction = reinterpret_cast<DeviceListingTransaction*>(transactionHandle);

    std::transform(
        std::begin(transaction->devices), std::end(transaction->devices),
        out,
        [](const contactci::DeviceDescription &device) {
            return ::DeviceDescription {
                .productLine = device.get_product_line().c_str(),
                .serialNumber = device.get_serial_number().c_str(),
                .isRight = device.get_is_right(),
                .isConnected = device.get_is_connected()
            };
        }
    );
}

void cci_end_device_listing_transaction(DeviceListingTransactionHandle transactionHandle) {
    auto *transaction = reinterpret_cast<DeviceListingTransaction*>(transactionHandle);
    delete transaction;
}

ClientListingTransactionHandle cci_start_client_listing_transaction() {
    return new ClientListingTransaction;
}

size_t cci_fetch_client_listing(CciSessionHandle sessionHandle, ClientListingTransactionHandle transactionHandle) {
    auto* session = reinterpret_cast<Session*>(sessionHandle);
    auto *transaction = reinterpret_cast<ClientListingTransaction*>(transactionHandle);

    transaction->clients = session->get_client_list();

    return transaction->clients.size();
}

void cci_get_client_listing(ClientListingTransactionHandle transactionHandle, ::ClientDescription* out) {
    auto *transaction = reinterpret_cast<ClientListingTransaction*>(transactionHandle);

    std::transform(
        std::begin(transaction->clients), std::end(transaction->clients),
        out,
        [](const contactci::ClientDescription &client) {
            return ::ClientDescription {
                .processID = client.get_process_id(),
                .processName = client.get_process_name().c_str()
            };
        }
    );
}

void cci_end_client_listing_transaction(ClientListingTransactionHandle transactionHandle) {
    auto *transaction = reinterpret_cast<ClientListingTransaction*>(transactionHandle);
    delete transaction;
}

ServiceInfoTransactionHandle cci_start_service_info_transaction() {
    return new ServiceInfoTransaction;
}

void cci_fetch_service_info(CciSessionHandle sessionHandle, ServiceInfoTransactionHandle transactionHandle) {
    auto* session = reinterpret_cast<Session*>(sessionHandle);
    auto *transaction = reinterpret_cast<ServiceInfoTransaction*>(transactionHandle);

    transaction->info = session->get_service_info();
}

void cci_get_service_info(ServiceInfoTransactionHandle transactionHandle, ::ServiceInfo *out) {
    auto *transaction = reinterpret_cast<ServiceInfoTransaction*>(transactionHandle);

    out->version = transaction->info->get_version().c_str();
    out->isInteractive = transaction->info->get_is_interactive();
}

void cci_end_service_info_transaction(ServiceInfoTransactionHandle transactionHandle) {
    auto *transaction = reinterpret_cast<ServiceInfoTransaction*>(transactionHandle);
    delete transaction;
}

CciSessionHandle cci_create_session() {
    try {
        return new Session();
    } catch (...) {
        return nullptr;
    }
}

CciSessionHandle cci_create_readonly_haptic_session() {
    try {
        return new HapticSession();
    } catch (...) {
        return nullptr;
    }
}

CciSessionHandle cci_create_mutable_haptic_session() {
    try {
        return new MutableHapticSession();
    } catch (...) {
        return nullptr;
    }
}

void cci_close_session(CciSessionHandle sessionHandle) {
    auto* session = reinterpret_cast<Session*>(sessionHandle);
    delete session;
}

bool cci_get_session_haptic_state(CciSessionHandle sessionHandle, HapticState** left, HapticState** right) {
    try {
        auto* session = reinterpret_cast<MutableHapticSession*>(sessionHandle);
        MutableHapticStateManager &state_manager = session->get_session_haptic_state();

        *left = &state_manager.get_left_haptic_state();
        *right = &state_manager.get_right_haptic_state();
        return true;
    } catch (...) {
        return false;
    }
}

bool cci_get_global_haptic_state(CciSessionHandle sessionHandle, const HapticState** left, const HapticState** right) {
    try {
        auto* session = reinterpret_cast<HapticSession*>(sessionHandle);
        const HapticStateManager &state_manager = session->get_global_haptic_state();

        *left = &state_manager.get_left_haptic_state();
        *right = &state_manager.get_right_haptic_state();
        return true;
    } catch (...) {
        return false;
    }
}

bool cci_signal_session_haptic_state_changed(CciSessionHandle sessionHandle) {
    try {
        auto* session = reinterpret_cast<MutableHapticSession*>(sessionHandle);
        MutableHapticStateManager &state_manager = session->get_session_haptic_state();

        state_manager.signal_haptic_state_changed();
        return true;
    } catch (...) {
        return false;
    }
}

bool cci_wait_global_haptic_state_changed(CciSessionHandle sessionHandle, int timeout_ms) {
    //TODO error handle if wait throws exception
    try {
        auto* session = reinterpret_cast<HapticSession*>(sessionHandle);
        const HapticStateManager &state_manager = session->get_global_haptic_state();

        state_manager.wait_for_state_change(timeout_ms);
        return true;
    } catch (...) {
        return false;
    }
}
