//
// Created by john_contactci on 4/30/2024.
//

#pragma once

typedef enum CciStatus {
    CCI_SUCCESS = 0,
    CCI_NO_DEVICE,
    CCI_UNKNOWN_ERROR,

    CCI_ERR_SESSION_INVALID_HANDLE,
    CCI_ERR_SESSION_NOT_CONNECTED,
    CCI_ERR_SESSION_ACCESS_DENIED,

    CCI_ERR_PIPE_FAILED_TO_OPEN,
    CCI_ERR_PIPE_FAILED_TO_WRITE,
    CCI_ERR_PIPE_FAILED_TO_READ,
    CCI_ERR_PIPE_FAILED_TO_FLUSH,

    CCI_ERR_EVENT_FAILED_TO_OPEN,
    CCI_ERR_EVENT_FAILED_TO_SET,
    CCI_ERR_EVENT_FAILED_TO_RESET,
    CCI_ERR_EVENT_FAILED_TO_WAIT,

    CCI_ERR_SHM_FAILED_TO_OPEN,
    CCI_ERR_SHM_FAILED_TO_MAP
} CciStatus;

static const char *cci_error_messages[] = {
    "Success",
    "No device exists",
    "Unknown error",

    "Invalid session handle",
    "Session not connected",
    "Session initialization failed due to service rejecting requested access",

    "Failed to open pipe",
    "Failed to write to pipe",
    "Failed to read from pipe",
    "Failed to flush pipe",

    "Failed to open event",
    "Failed to set event",
    "Failed to reset event",
    "Failed to wait on event",

    "Failed to open shared memory",
    "Failed to map view of shared memory"
};

static const char *get_error_string(CciStatus error) {
    if (error < 0 || error >= (sizeof(cci_error_messages) / sizeof(*cci_error_messages)))
        return nullptr;

    return cci_error_messages[error];
}