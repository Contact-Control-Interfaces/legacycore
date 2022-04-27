//
// Created by john_contactci on 4/11/2022.
//

#ifndef CONTACTCI_CORE_H
#define CONTACTCI_CORE_H

#ifdef _WIN32
#    ifdef LIBRARY_EXPORTS
#        define LIBRARY_API extern "C" __declspec(dllexport)
#    else
#        define LIBRARY_API __declspec(dllimport)
#    endif
#elif
#    ifdef LIBRARY_EXPORTS
#        define LIBRARY_API extern "C"
#    else
#        define LIBRARY_API
#    endif
#endif

// TODO move to another header?
typedef void* CommunicatorHandle;

LIBRARY_API void cci_update(CommunicatorHandle comms);

#endif //CONTACTCI_CORE_H
