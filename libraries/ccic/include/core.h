//
// Created by john_contactci on 4/11/2022.
//

#ifndef CONTACTCI_CORE_H
#define CONTACTCI_CORE_H

#include <cci_lib_defs.h>

#include <stdlib.h>

// TODO move to another header?
typedef void* ChannelHandle;

CCI_API_FUNC(ChannelHandle) cci_channel_unity_console_create(void (*callback)(const char*, size_t length));
CCI_API_FUNC(ChannelHandle) cci_channel_stdout_create();
CCI_API_FUNC(ChannelHandle) cci_channel_pipe_create();

CCI_API_FUNC(void) cci_channel_destroy(ChannelHandle channel);

CCI_API_FUNC(void) cci_channel_send_set_dimension_message(ChannelHandle channel, unsigned int dimension, unsigned int flags, unsigned int bitmask, const char* values, int valuesCount);

#endif //CONTACTCI_CORE_H
