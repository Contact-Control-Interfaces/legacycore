//
// Created by john_contactci on 4/11/2022.
//

#ifndef CONTACTCI_CORE_H
#define CONTACTCI_CORE_H

//This handles this header with C++ compilers
#ifdef __cplusplus
#   define CCI_EXTERN_C extern "C"
#else
#   define CCI_EXTERN_C
#endif

#define WIN_OR_CYGWIN ( \
    defined(__WINDOWS__) \
    || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) \
    || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) \
    || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__CYGWIN64__) \
)

#if WIN_OR_CYGWIN
#   define CCI_DLL_EXPORT __declspec(dllexport)
#   define CCI_DLL_IMPORT __declspec(dllimport)
#else
#   define CCI_DLL_EXPORT
#   define CCI_DLL_IMPORT
#endif

#define CCI_CALL_CONV __stdcall

#ifdef CCI_API_EXPORT
#   define CCI_LIB_ATTR CCI_DLL_EXPORT
#else
#   define CCI_LIB_ATTR CCI_DLL_IMPORT
#endif

#define CCI_API(x) CCI_EXTERN_C CCI_LIB_ATTR x CCI_CALL_CONV

// TODO move to another header?
typedef void* CommunicatorHandle;

CCI_API(void) cci_update(CommunicatorHandle comms);

#endif //CONTACTCI_CORE_H
