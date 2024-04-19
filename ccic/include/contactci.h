//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <stddef.h>

#include <types.h>
#include <lib_defs.h>

typedef void* CciSessionHandle;

typedef void* DeviceListingTransactionHandle;
typedef void* ClientListingTransactionHandle;

CCI_API_FUNC(DeviceListingTransactionHandle) cci_start_device_listing_transaction();
CCI_API_FUNC(size_t) cci_fetch_device_listing(CciSessionHandle sessionHandle, DeviceListingTransactionHandle transactionHandle);
CCI_API_FUNC(void) cci_get_device_listing(DeviceListingTransactionHandle transactionHandle, DeviceDescription *out);
CCI_API_FUNC(void) cci_end_device_listing_transaction(DeviceListingTransactionHandle transactionHandle);

CCI_API_FUNC(bool) cci_get_left_device(CciSessionHandle sessionHandle, DeviceDescription *out);
CCI_API_FUNC(bool) cci_get_right_device(CciSessionHandle sessionHandle, DeviceDescription *out);

CCI_API_FUNC(ClientListingTransactionHandle) cci_start_client_listing_transaction();
CCI_API_FUNC(size_t) cci_fetch_client_listing(CciSessionHandle sessionHandle, ClientListingTransactionHandle transactionHandle);
CCI_API_FUNC(void) cci_get_client_listing(ClientListingTransactionHandle transactionHandle, ClientDescription* out);
CCI_API_FUNC(void) cci_end_client_listing_transaction(ClientListingTransactionHandle transactionHandle);

CCI_API_FUNC(CciSessionHandle) cci_create_session();
CCI_API_FUNC(CciSessionHandle) cci_create_readonly_haptic_session();
CCI_API_FUNC(CciSessionHandle) cci_create_mutable_haptic_session();
CCI_API_FUNC(void) cci_close_session(CciSessionHandle sessionHandle);

CCI_API_FUNC(bool) cci_get_session_haptic_state(CciSessionHandle sessionHandle, HapticState** left, HapticState** right);
CCI_API_FUNC(bool) cci_get_global_haptic_state(CciSessionHandle sessionHandle, const HapticState** left, const HapticState** right);

CCI_API_FUNC(bool) cci_signal_session_haptic_state_changed(CciSessionHandle sessionHandle);
CCI_API_FUNC(bool) cci_wait_global_haptic_state_changed(CciSessionHandle sessionHandle, int timeout_ms);

CCI_API_FUNC(const char*) cci_get_session_service_version(CciSessionHandle sessionHandle);
CCI_API_FUNC(bool) cci_is_session_service_interactive(CciSessionHandle sessionHandle);
