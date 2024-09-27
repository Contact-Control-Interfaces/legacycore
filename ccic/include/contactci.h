//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <ccic/lib_defs.h>
#include <ccic/error.h>
#include <ccic/haptic_state.h>
#include <cci/cached_waveform.h>

typedef void* CciSessionHandle;
typedef void* DeviceListingTransactionHandle;
typedef void* ClientListingTransactionHandle;

typedef struct ClientDescription {
    uint32_t processID;
    const char* processName;
} ClientDescription;

typedef struct DeviceDescription {
    const char* productLine;
    const char* serialNumber;
    bool isRight;
    bool isConnected;
} DeviceDescription;

CCI_API_FUNC(DeviceListingTransactionHandle) cci_start_device_listing_transaction();
CCI_API_FUNC(CciStatus) cci_fetch_device_listing(CciSessionHandle sessionHandle, DeviceListingTransactionHandle transactionHandle, size_t *count);
CCI_API_FUNC(void) cci_get_device_listing(DeviceListingTransactionHandle transactionHandle, DeviceDescription *out);
CCI_API_FUNC(void) cci_end_device_listing_transaction(DeviceListingTransactionHandle transactionHandle);

CCI_API_FUNC(CciStatus) cci_get_left_device(CciSessionHandle sessionHandle, DeviceDescription *out);
CCI_API_FUNC(CciStatus) cci_get_right_device(CciSessionHandle sessionHandle, DeviceDescription *out);

CCI_API_FUNC(ClientListingTransactionHandle) cci_start_client_listing_transaction();
CCI_API_FUNC(CciStatus) cci_fetch_client_listing(CciSessionHandle sessionHandle, ClientListingTransactionHandle transactionHandle, size_t *count);
CCI_API_FUNC(void) cci_get_client_listing(ClientListingTransactionHandle transactionHandle, ClientDescription* out);
CCI_API_FUNC(void) cci_end_client_listing_transaction(ClientListingTransactionHandle transactionHandle);

CCI_API_FUNC(CciStatus) cci_create_session(CciSessionHandle *handle);
CCI_API_FUNC(CciStatus) cci_create_readonly_haptic_session(CciSessionHandle *handle);
CCI_API_FUNC(CciStatus) cci_create_mutable_haptic_session(CciSessionHandle *handle);
CCI_API_FUNC(void) cci_close_session(CciSessionHandle sessionHandle);
CCI_API_FUNC(CciStatus) cci_is_session_connected(CciSessionHandle sessionHandle);

CCI_API_FUNC(CciStatus) cci_get_session_haptic_state(CciSessionHandle sessionHandle, HapticState** left, HapticState** right);
CCI_API_FUNC(CciStatus) cci_get_global_haptic_state(CciSessionHandle sessionHandle, const HapticState** left, const HapticState** right);

CCI_API_FUNC(CciStatus) cci_signal_session_haptic_state_changed(CciSessionHandle sessionHandle);
CCI_API_FUNC(CciStatus) cci_wait_global_haptic_state_changed(CciSessionHandle sessionHandle, int timeout_ms, bool *stateChanged);

CCI_API_FUNC(const char*) cci_get_session_service_version(CciSessionHandle sessionHandle);
CCI_API_FUNC(bool) cci_is_session_service_interactive(CciSessionHandle sessionHandle);

CCI_API_FUNC(const char*) cci_get_error_string(CciStatus status);

CCI_API_FUNC(CciStatus) cci_get_cached_waveforms(CciSessionHandle sessionHandle, ::DeviceDescription* device, bool terse, CachedWaveform** waves, int* length);
CCI_API_FUNC(void) cci_delete_waveform(CciSessionHandle sessionHandle, ::DeviceDescription* device, int index);
CCI_API_FUNC(void) cci_delete_all_waveform(CciSessionHandle sessionHandle, ::DeviceDescription* device);
CCI_API_FUNC(CciStatus) cci_transfer_waveform(CciSessionHandle sessionHandle, ::DeviceDescription* device, uint32_t sample_rate, uint8_t modifiers, float* samples,
                                                int numsamples, char* description, CachedWaveform* cache_out, WaveformTransferResponse* response_out);
