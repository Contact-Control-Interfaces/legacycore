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
    } catch (std::exception &e) {
        return nullptr;
    }
}

void cci_close_session(CciSessionHandle sessionHandle) {
    auto* session = reinterpret_cast<Session*>(sessionHandle);
    delete session;
}

bool cci_get_session_haptic_state(CciSessionHandle sessionHandle, HapticState** left, HapticState** right) {
    try {
        auto* session = dynamic_cast<MutableHapticSession*>(reinterpret_cast<Session*>(sessionHandle));

        if (session == nullptr)
            return false;

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
        auto* session = dynamic_cast<HapticSession*>(reinterpret_cast<Session*>(sessionHandle));

        if (session == nullptr)
            return false;

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
        auto* session = dynamic_cast<MutableHapticSession*>(reinterpret_cast<Session*>(sessionHandle));

        if (session == nullptr)
            return false;

        MutableHapticStateManager &state_manager = session->get_session_haptic_state();

        state_manager.signal_haptic_state_changed();
        return true;
    } catch (...) {
        return false;
    }
}

bool cci_wait_global_haptic_state_changed(CciSessionHandle sessionHandle, int timeout_ms) {
    try {
        auto* session = dynamic_cast<HapticSession*>(reinterpret_cast<Session*>(sessionHandle));

        if (session == nullptr)
            return false;

        const HapticStateManager &state_manager = session->get_global_haptic_state();

        return state_manager.wait_for_state_change(timeout_ms);
    } catch (...) {
        return false;
    }
}

const char* cci_get_session_service_version(CciSessionHandle sessionHandle) {
    auto* session = reinterpret_cast<Session*>(sessionHandle);

    return session->get_service_version().c_str();
}

bool cci_is_session_service_interactive(CciSessionHandle sessionHandle) {
    auto* session = reinterpret_cast<Session*>(sessionHandle);

    return session->is_service_interactive();
}
