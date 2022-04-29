//
// Created by john_contactci on 4/11/2022.
//

#ifndef CONTACTCI_CORE_H
#define CONTACTCI_CORE_H

#include <cci_lib_defs.h>

#include <stdlib.h>

// TODO move to another header?
typedef void* CommunicatorHandle;

CCI_API_FUNC(CommunicatorHandle) cci_communicator_stdout_create();
CCI_API_FUNC(CommunicatorHandle) cci_communicator_unity_console_create(void (*callback)(const char*, size_t length));
CCI_API_FUNC(CommunicatorHandle) cci_communicator_bluetooth_create();
CCI_API_FUNC(CommunicatorHandle) cci_communicator_usb_serial_create();

CCI_API_FUNC(void) cci_communicator_destroy(CommunicatorHandle comms);

CCI_API_FUNC(void) cci_update(CommunicatorHandle comms);

#endif //CONTACTCI_CORE_H
