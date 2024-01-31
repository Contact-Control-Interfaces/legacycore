//
// Created by zach_contactci on 9/11/2023.
//

#ifndef CONTACTCI_INFO_H
#define CONTACTCI_INFO_H

#include <cci_lib_defs.h>

#include "info_types.h"

typedef void* ChannelHandle;
typedef void* DeviceListingTransactionHandle;
typedef void* ClientListingTransactionHandle;
typedef void* ServiceInfoTransactionHandle;

CCI_API_FUNC(DeviceListingTransactionHandle) cci_start_device_listing_transaction();
CCI_API_FUNC(void) cci_fetch_device_listing(ChannelHandle channelHandle, DeviceListingTransactionHandle transactionHandle);
CCI_API_FUNC(size_t) cci_get_device_listing(DeviceListingTransactionHandle transactionHandle, DeviceDescription **deviceDescriptionsOut);
CCI_API_FUNC(void) cci_end_device_listing_transaction(DeviceListingTransactionHandle transactionHandle);

CCI_API_FUNC(ClientListingTransactionHandle) cci_start_client_listing_transaction();
CCI_API_FUNC(void) cci_fetch_client_listing(ChannelHandle channelHandle, ClientListingTransactionHandle transactionHandle);
CCI_API_FUNC(size_t) cci_get_client_listing(ClientListingTransactionHandle transactionHandle, ClientDescription **clientDescriptionsOut);
CCI_API_FUNC(void) cci_end_client_listing_transaction(ClientListingTransactionHandle transactionHandle);

CCI_API_FUNC(ServiceInfoTransactionHandle) cci_start_service_info_transaction();
CCI_API_FUNC(void) cci_fetch_service_info(ChannelHandle channelHandle, ServiceInfoTransactionHandle transactionHandle);
CCI_API_FUNC(void) cci_get_service_info(ServiceInfoTransactionHandle transactionHandle, ServiceInfo **serviceInfoOut);
CCI_API_FUNC(void) cci_end_service_info_transaction(ServiceInfoTransactionHandle transactionHandle);

#endif //CONTACTCI_INFO_H
