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

CCI_API_FUNC(DeviceListingTransactionHandle) cci_start_device_listing_transaction();
CCI_API_FUNC(void) cci_fetch_device_listing(ChannelHandle channelHandle, DeviceListingTransactionHandle deviceListing);
CCI_API_FUNC(size_t) cci_get_device_listing(DeviceListingTransactionHandle deviceListing, DeviceDescription **deviceDescriptionsOut);
CCI_API_FUNC(void) cci_end_device_listing_transaction(DeviceListingTransactionHandle deviceListing);

CCI_API_FUNC(ClientListingTransactionHandle) cci_start_client_listing_transaction();
CCI_API_FUNC(void) cci_fetch_client_listing(ChannelHandle channelHandle, ClientListingTransactionHandle clientListing);
CCI_API_FUNC(size_t) cci_get_client_listing(ClientListingTransactionHandle clientListing, ClientDescription **clientDescriptionsOut);
CCI_API_FUNC(void) cci_end_client_listing_transaction(ClientListingTransactionHandle clientListing);

#endif //CONTACTCI_INFO_H
