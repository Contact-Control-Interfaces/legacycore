//
// Created by john_contactci on 3/18/2024.
//

#include "winerr_util.h"

#include <windows.h>

std::string GetLastErrorAsString() {
    DWORD errorMessageID = GetLastError();

    if (errorMessageID == 0)
        return {};

    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr
    );

    std::string message(messageBuffer, size);

    LocalFree(messageBuffer);

    return message;
}