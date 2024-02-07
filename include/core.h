//
// Created by john_contactci on 4/11/2022.
//

#pragma once

#include <cci_lib_defs.h>

#include <stdlib.h>

typedef void(*log_callback)(const char*, size_t length);

// TODO move to another header?
typedef void* ChannelHandle;

CCI_API_FUNC(ChannelHandle) cci_channel_unity_console_create(void (*callback)(const char*, size_t length));
CCI_API_FUNC(ChannelHandle) cci_channel_stdout_create();
CCI_API_FUNC(ChannelHandle) cci_channel_pipe_create();

CCI_API_FUNC(void) cci_channel_destroy(ChannelHandle channel);

CCI_API_FUNC(void) cci_channel_send_set_dimension_message(ChannelHandle channel, unsigned int dimension, unsigned int flags, unsigned int bitmask, const char* values, int valuesCount);

CCI_API_FUNC(bool) is_ble_processing();

CCI_API_FUNC(void) install_log_callback(log_callback callback);
