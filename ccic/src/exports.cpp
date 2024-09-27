//
// Created by john_contactci on 3/9/2022.
//
#include <algorithm>
#include <optional>

#include "contactci.h"

#include <cci/error.h>
#include <cci/session.h>

#define CCI_ERROR_WRAP(x)               \
    try {                               \
        x                               \
    } catch (contactci::Exception &e) { \
        return e.get_error_code();      \
    } catch (...) {                     \
        return CCI_UNKNOWN_ERROR;       \
    }                                   \

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

CciStatus cci_fetch_device_listing(CciSessionHandle sessionHandle, DeviceListingTransactionHandle transactionHandle, size_t *count) {
    CCI_ERROR_WRAP(
        auto* session = static_cast<Session*>(sessionHandle);
        auto *transaction = static_cast<DeviceListingTransaction*>(transactionHandle);

        transaction->devices = session->get_device_list();

        *count = transaction->devices.size();
        return CCI_SUCCESS;
    )
}

static ::DeviceDescription map_device_description(const contactci::DeviceDescription &device) {
    return ::DeviceDescription {
        .productLine = device.get_product_line().c_str(),
        .serialNumber = device.get_serial_number().c_str(),
        .isRight = device.get_is_right(),
        .isConnected = device.get_is_connected()
    };
}

void cci_get_device_listing(DeviceListingTransactionHandle transactionHandle, ::DeviceDescription *out) {
    auto *transaction = static_cast<DeviceListingTransaction*>(transactionHandle);

    std::transform(
        std::begin(transaction->devices), std::end(transaction->devices),
        out,
        map_device_description
    );
}

void cci_end_device_listing_transaction(DeviceListingTransactionHandle transactionHandle) {
    auto *transaction = static_cast<DeviceListingTransaction*>(transactionHandle);
    delete transaction;
}

CciStatus cci_get_left_device(CciSessionHandle sessionHandle, ::DeviceDescription *out) {
    CCI_ERROR_WRAP(
        auto* session = static_cast<Session*>(sessionHandle);
        const std::optional<contactci::DeviceDescription> &leftDevice = session->get_left_device();

        if (!leftDevice.has_value())
            return CCI_NO_DEVICE;

        if (out != nullptr)
            *out = map_device_description(leftDevice.value());

        return CCI_SUCCESS;
    )
}

CciStatus cci_get_right_device(CciSessionHandle sessionHandle, ::DeviceDescription *out) {
    CCI_ERROR_WRAP(
        auto* session = static_cast<Session*>(sessionHandle);
        const std::optional<contactci::DeviceDescription> &rightDevice = session->get_right_device();

        if (!rightDevice.has_value())
            return CCI_NO_DEVICE;

        if (out != nullptr)
            *out = map_device_description(rightDevice.value());

        return CCI_SUCCESS;
    )
}

ClientListingTransactionHandle cci_start_client_listing_transaction() {
    return new ClientListingTransaction;
}

CciStatus cci_fetch_client_listing(CciSessionHandle sessionHandle, ClientListingTransactionHandle transactionHandle, size_t *count) {
    CCI_ERROR_WRAP(
        auto* session = static_cast<Session*>(sessionHandle);
        auto *transaction = static_cast<ClientListingTransaction*>(transactionHandle);

        transaction->clients = session->get_client_list();

        *count = transaction->clients.size();
        return CCI_SUCCESS;
    )
}

void cci_get_client_listing(ClientListingTransactionHandle transactionHandle, ::ClientDescription* out) {
    auto *transaction = static_cast<ClientListingTransaction*>(transactionHandle);

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
    auto *transaction = static_cast<ClientListingTransaction*>(transactionHandle);
    delete transaction;
}

CciStatus cci_create_session(CciSessionHandle *handle) {
    CCI_ERROR_WRAP(
       *handle = new Session();
       return CCI_SUCCESS;
   )
}

CciStatus cci_create_readonly_haptic_session(CciSessionHandle *handle) {
    CCI_ERROR_WRAP(
        *handle = new HapticSession();
        return CCI_SUCCESS;
    )
}

CciStatus cci_create_mutable_haptic_session(CciSessionHandle *handle) {
    CCI_ERROR_WRAP(
        *handle = new MutableHapticSession();
        return CCI_SUCCESS;
    )
}

void cci_close_session(CciSessionHandle sessionHandle) {
    auto* session = static_cast<Session*>(sessionHandle);
    delete session;
}

CciStatus cci_is_session_connected(CciSessionHandle sessionHandle) {
    CCI_ERROR_WRAP(
        auto* session = static_cast<Session*>(sessionHandle);
        return session->is_connected() ? CCI_SUCCESS : CCI_SESSION_NOT_CONNECTED;
    )
}

CciStatus cci_get_session_haptic_state(CciSessionHandle sessionHandle, HapticState** left, HapticState** right) {
    CCI_ERROR_WRAP(
        auto* session = dynamic_cast<MutableHapticSession*>(static_cast<Session*>(sessionHandle));

        if (session == nullptr)
            return CCI_ERR_SESSION_INVALID_HANDLE;

        MutableHapticStateManager &state_manager = session->get_session_haptic_state();

        *left = &state_manager.get_left_haptic_state();
        *right = &state_manager.get_right_haptic_state();
        return CCI_SUCCESS;
    )
}

CciStatus cci_get_global_haptic_state(CciSessionHandle sessionHandle, const HapticState** left, const HapticState** right) {
    CCI_ERROR_WRAP(
        auto* session = dynamic_cast<HapticSession*>(static_cast<Session*>(sessionHandle));

        if (session == nullptr)
            return CCI_ERR_SESSION_INVALID_HANDLE;

        const HapticStateManager &state_manager = session->get_global_haptic_state();

        *left = &state_manager.get_left_haptic_state();
        *right = &state_manager.get_right_haptic_state();
        return CCI_SUCCESS;
    )
}

CciStatus cci_signal_session_haptic_state_changed(CciSessionHandle sessionHandle) {
    CCI_ERROR_WRAP(
        auto* session = dynamic_cast<MutableHapticSession*>(static_cast<Session*>(sessionHandle));

        if (session == nullptr)
            return CCI_ERR_SESSION_INVALID_HANDLE;

        MutableHapticStateManager &state_manager = session->get_session_haptic_state();
        state_manager.signal_haptic_state_changed();
        return CCI_SUCCESS;
    )
}

CciStatus cci_wait_global_haptic_state_changed(CciSessionHandle sessionHandle, int timeout_ms, bool *stateChanged) {
    CCI_ERROR_WRAP(
        auto* session = dynamic_cast<HapticSession*>(static_cast<Session*>(sessionHandle));

        if (session == nullptr)
            return CCI_ERR_SESSION_INVALID_HANDLE;

        const HapticStateManager &state_manager = session->get_global_haptic_state();
        *stateChanged = state_manager.wait_for_state_change(timeout_ms);
        return CCI_SUCCESS;
    )
}

const char* cci_get_session_service_version(CciSessionHandle sessionHandle) {
    auto* session = static_cast<Session*>(sessionHandle);
    return session->get_service_version().c_str();
}

bool cci_is_session_service_interactive(CciSessionHandle sessionHandle) {
    auto* session = static_cast<Session*>(sessionHandle);
    return session->is_service_interactive();
}

const char* cci_get_error_string(CciStatus status) {
    return get_error_string(status);
}

CciStatus cci_get_cached_waveforms(CciSessionHandle sessionHandle, ::DeviceDescription* device, bool terse, CachedWaveform** waves, int* length) {
    CCI_ERROR_WRAP(
        auto* session = dynamic_cast<HapticSession*>(static_cast<Session*>(sessionHandle));

        if (session == nullptr)
        return CCI_ERR_SESSION_INVALID_HANDLE;

        const auto &manager = session->get_global_device_manager();
        auto list = manager.get_cached_waveforms(device->serialNumber, terse);
        *waves = list.data();
        *length = static_cast<int>(list.size());
    return CCI_SUCCESS;
    )
}

void cci_delete_waveform(CciSessionHandle sessionHandle, ::DeviceDescription* device, int index) {
    auto* session = dynamic_cast<MutableHapticSession*>(static_cast<Session*>(sessionHandle));

    if (session == nullptr)
        return;

    const auto &manager = session->get_session_device_manager();
    manager.delete_waveform(device->serialNumber, index);
}

void cci_delete_all_waveform(CciSessionHandle sessionHandle, ::DeviceDescription* device) {
    auto* session = dynamic_cast<MutableHapticSession*>(static_cast<Session*>(sessionHandle));

    if (session == nullptr)
        return;

    const auto &manager = session->get_session_device_manager();
    manager.delete_all_waveform(device->serialNumber);
}

CciStatus cci_transfer_waveform(CciSessionHandle sessionHandle, ::DeviceDescription* device, uint32_t sample_rate, uint8_t modifiers, float* samples, int numsamples, char* description, CachedWaveform* cache_out, WaveformTransferResponse* response_out) {
    CCI_ERROR_WRAP(
    auto* session = dynamic_cast<MutableHapticSession*>(static_cast<Session*>(sessionHandle));

    if (session == nullptr)
        return CCI_ERR_SESSION_INVALID_HANDLE;

    const auto &manager = session->get_session_device_manager();
    auto vsamp = std::vector<float>(samples, samples + numsamples);
    std::optional<std::string> desc;
    if(description)
        desc = description;
    else
        desc = std::nullopt;
    *response_out = manager.transfer_waveform(device->serialNumber, sample_rate, modifiers, vsamp, *cache_out, desc);
    return CCI_SUCCESS;
    )
}
