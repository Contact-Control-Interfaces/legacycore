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
typedef void* ServiceInfoTransactionHandle;

CCI_API_FUNC(DeviceListingTransactionHandle) cci_start_device_listing_transaction();
CCI_API_FUNC(size_t) cci_fetch_device_listing(CciSessionHandle sessionHandle, DeviceListingTransactionHandle transactionHandle);
CCI_API_FUNC(void) cci_get_device_listing(DeviceListingTransactionHandle transactionHandle, DeviceDescription *out);
CCI_API_FUNC(void) cci_end_device_listing_transaction(DeviceListingTransactionHandle transactionHandle);

CCI_API_FUNC(ClientListingTransactionHandle) cci_start_client_listing_transaction();
CCI_API_FUNC(size_t) cci_fetch_client_listing(CciSessionHandle sessionHandle, ClientListingTransactionHandle transactionHandle);
CCI_API_FUNC(void) cci_get_client_listing(ClientListingTransactionHandle transactionHandle, ClientDescription* out);
CCI_API_FUNC(void) cci_end_client_listing_transaction(ClientListingTransactionHandle transactionHandle);

CCI_API_FUNC(ServiceInfoTransactionHandle) cci_start_service_info_transaction();
CCI_API_FUNC(void) cci_fetch_service_info(CciSessionHandle sessionHandle, ServiceInfoTransactionHandle transactionHandle);
CCI_API_FUNC(void) cci_get_service_info(ServiceInfoTransactionHandle transactionHandle, ServiceInfo *out);
CCI_API_FUNC(void) cci_end_service_info_transaction(ServiceInfoTransactionHandle transactionHandle);

CCI_API_FUNC(CciSessionHandle) cci_create_session();
CCI_API_FUNC(void) cci_close_session(CciSessionHandle sessionHandle);

CCI_API_FUNC(bool) cci_attach_haptic_state(CciSessionHandle sessionHandle, bool observeOnly);
CCI_API_FUNC(void) cci_detach_haptic_state(CciSessionHandle sessionHandle);

CCI_API_FUNC(bool) cci_get_session_haptic_state(CciSessionHandle sessionHandle, HapticState** left, HapticState** right);
CCI_API_FUNC(bool) cci_get_global_haptic_state(CciSessionHandle sessionHandle, HapticState** left, HapticState** right);

CCI_API_FUNC(bool) cci_signal_session_haptic_state_changed(CciSessionHandle sessionHandle);
CCI_API_FUNC(bool) cci_wait_global_haptic_state_changed(CciSessionHandle sessionHandle, int timeout_ms);
