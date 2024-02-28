//
// Created by john_contactci on 2/21/2024.
//

#include <stdexcept>
#include <string>

#include "shared_memory.h"

#include <winternl.h>
#include <AclAPI.h>

using namespace contactci::io;

// Unnamed namespace rather than C-style static functions
// See https://stackoverflow.com/questions/154469/why-should-you-prefer-unnamed-namespaces-over-static-functions
namespace {
    std::string GetLastErrorAsString() {
        DWORD errorMessageID = GetLastError();

        if (errorMessageID == 0)
            return {};

        LPSTR messageBuffer = nullptr;

        //Ask Win32 to give us the string version of that message ID.
        //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL
        );

        std::string message(messageBuffer, size);

        LocalFree(messageBuffer);

        return message;
    }

    HANDLE create_event(LPCSTR eventName) {
        HANDLE result;
        PSECURITY_DESCRIPTOR securityDescriptor = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
        InitializeSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(securityDescriptor, TRUE, NULL, FALSE);

        SECURITY_ATTRIBUTES securityAttributes = {};
        securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        securityAttributes.lpSecurityDescriptor = securityDescriptor;
        securityAttributes.bInheritHandle = FALSE;

        // If event already exists it'll just return a handle to it and set last error to ERROR_ALREADY_EXISTS
        result = CreateEvent(&securityAttributes, TRUE, FALSE, eventName);

        if (result == NULL) {
            throw std::runtime_error(
                    std::string("Failed to open event: OpenEvent; GetLastError = ")
                    + GetLastErrorAsString()
            );
        }

        return result;
    }

    HANDLE open_shared_memory(std::size_t memory_size, LPCSTR name) {
        HANDLE result;
//        PSID everyoneSid;
//        PACL acl;
//        SID_IDENTIFIER_AUTHORITY sidAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
//        EXPLICIT_ACCESS access;
//        PSECURITY_DESCRIPTOR securityDescriptor;
//        SECURITY_ATTRIBUTES securityAttributes;
//
//        if (!AllocateAndInitializeSid(&sidAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyoneSid)) {
//            throw std::runtime_error(
//                    std::string("Failed to create \"everyone\" security identifier: AllocateAndInitializeSid; GetLastError = ")
//                    + GetLastErrorAsString()
//            );
//        }
//
//        ZeroMemory(&access, sizeof(EXPLICIT_ACCESS));
//
//        access.grfAccessPermissions = FILE_ALL_ACCESS;
//        access.grfAccessMode = SET_ACCESS;
//        access.grfInheritance = NO_INHERITANCE;
//        access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
//        access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
//        access.Trustee.ptstrName = (LPTSTR)everyoneSid;
//
//        if (SetEntriesInAcl(1, &access, NULL, &acl) != ERROR_SUCCESS) {
//            throw std::runtime_error(
//                    std::string("Failed to create access control list: SetEntriesInAcl; GetLastError = ")
//                    + GetLastErrorAsString()
//            );
//        }
//
//        securityDescriptor = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
//
//        if (securityDescriptor == NULL) {
//            throw std::runtime_error(
//                    std::string("Failed to allocate security descriptor: LocalAlloc; GetLastError = ")
//                    + GetLastErrorAsString()
//            );
//        }
//
//        if (!InitializeSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION)) {
//            throw std::runtime_error(
//                    std::string("Failed to initialize security descriptor: InitializeSecurityDescriptor; GetLastError = ")
//                    + GetLastErrorAsString()
//            );
//        }
//
//        if (!SetSecurityDescriptorDacl(securityDescriptor, TRUE, acl, FALSE)) {
//            throw std::runtime_error(
//                    std::string("Failed to set security descriptor ACL: SetSecurityDescriptorDacl; GetLastError = ")
//                    + GetLastErrorAsString()
//            );
//        }
        SECURITY_DESCRIPTOR securityDescriptor;
        SECURITY_ATTRIBUTES securityAttributes;

        securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        securityAttributes.lpSecurityDescriptor = &securityDescriptor;
        securityAttributes.bInheritHandle = FALSE;

        if (!InitializeSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION)) {
            throw std::runtime_error(
                    std::string("Failed to initialize security descriptor: InitializeSecurityDescriptor; GetLastError = ")
                    + GetLastErrorAsString()
            );
        }

        if (!SetSecurityDescriptorDacl(&securityDescriptor, TRUE, NULL, FALSE)) {
            throw std::runtime_error(
                    std::string("Failed to set security descriptor ACL: SetSecurityDescriptorDacl; GetLastError = ")
                    + GetLastErrorAsString()
            );
        }

        result = OpenFileMapping(PAGE_READWRITE, FALSE, name);

        if (result == NULL) {
            throw std::runtime_error(
                    std::string("Failed to open shared memory: OpenFileMapping; GetLastError = ")
                    + GetLastErrorAsString()
            );
        }
    }

    LPVOID create_shared_memory_view(HANDLE sharedMemoryHandle, std::size_t offset, std::size_t viewSize) {
        LPVOID result = MapViewOfFile(sharedMemoryHandle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, viewSize);

        if (result == NULL) {
            throw std::runtime_error(
                    std::string("Failed to map shared memory: MapViewOfFile; GetLastError = ")
                    + GetLastErrorAsString()
            );
        }

        return result;
    }
}

SharedMemoryManager::SharedMemoryManager(const std::string& memoryName, const std::string& leftEventName, const std::string& rightEventName) {
    std::size_t memorySize = sizeof(HapticState) * 2;

    sharedMemoryHandle = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, memoryName.c_str());

    if (sharedMemoryHandle == NULL) {
        throw std::runtime_error(
                std::string("Failed to open shared memory: OpenFileMapping; GetLastError = ")
                + GetLastErrorAsString()
        );
    }

    /*
     * It would be nice to create two separate views with one being offset by sizeof(HapticState)
     * in order to have isolated views of the left vs right HapticState. However, the offset passed to
     * MapViewOfFile needs to be a multiple of the "allocation granularity" (which is like 64kb), so instead
     * we just have to manually offset the pointer by sizeof(HapticState)
     */
    sharedMemory = MapViewOfFile(sharedMemoryHandle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, memorySize);

    if (sharedMemory == NULL) {
        throw std::runtime_error(
                std::string("Failed to map shared memory: MapViewOfFile; GetLastError = ")
                + GetLastErrorAsString()
        );
    }

    leftEventHandle = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, leftEventName.c_str());

    if (leftEventHandle == NULL) {
        throw std::runtime_error(
                std::string("Failed to open event: OpenEvent; GetLastError = ")
                + GetLastErrorAsString()
        );
    }

    rightEventHandle = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, rightEventName.c_str());

    if (rightEventHandle == NULL) {
        throw std::runtime_error(
                std::string("Failed to open event: OpenEvent; GetLastError = ")
                + GetLastErrorAsString()
        );
    }
}

SharedMemoryManager::~SharedMemoryManager() {
    UnmapViewOfFile(sharedMemory);
    sharedMemory = nullptr;
}

HapticState *SharedMemoryManager::getLeftHapticStateMapping() {
    return static_cast<HapticState*>(sharedMemory);
}

HapticState *SharedMemoryManager::getRightHapticStateMapping() {
    return static_cast<HapticState*>(sharedMemory) + 1;
}

void SharedMemoryManager::signalLeftEvent() {
    // Set and immediately unset event; this will wake all threads waiting on `eventHandle` and then they'll
    // Wait again after processing shared mem
    if (!SetEvent(leftEventHandle)) {
        throw std::runtime_error(
                std::string("Failed to signal event: SetEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }

    if (!ResetEvent(leftEventHandle)) {
        throw std::runtime_error(
                std::string("Failed to reset event: ResetEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }
}

void SharedMemoryManager::signalRightEvent() {
    // Set and immediately unset event; this will wake all threads waiting on `eventHandle` and then they'll
    // Wait again after processing shared mem
    if (!SetEvent(rightEventHandle)) {
        throw std::runtime_error(
                std::string("Failed to signal event: SetEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }

    if (!ResetEvent(rightEventHandle)) {
        throw std::runtime_error(
                std::string("Failed to reset event: ResetEvent; GetLastError = ")
                + std::to_string(GetLastError())
        );
    }
}