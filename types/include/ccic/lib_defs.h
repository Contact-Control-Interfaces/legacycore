//
// Created by john_contactci on 4/28/2022.
//

#pragma once

//This handles this header with C++ compilers
#ifdef __cplusplus
#   define CCI_EXTERN_C extern "C"
#else
#   define CCI_EXTERN_C
#endif

#if ( \
    defined(_MSC_VER) || defined(__WINDOWS__) \
    || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) \
    || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) \
    || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__CYGWIN64__) \
)
    #define CCI_WIN_OR_CYGWIN
#endif

#ifdef CCI_WIN_OR_CYGWIN
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

#ifdef CCIC_API_EXPORT
#   define CCIC_LIB_ATTR CCI_DLL_EXPORT
#else
#   define CCIC_LIB_ATTR CCI_DLL_IMPORT
#endif

#define CCI_API CCI_LIB_ATTR
#define CCI_API_FUNC(x) CCI_EXTERN_C CCIC_LIB_ATTR x CCI_CALL_CONV
#define CCI_API_CLASS(x) CCI_LIB_ATTR x
