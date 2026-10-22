#pragma once

// The service-side objects (TestEvent, TestSharedMemory, unique_name) for the
// platform being built: named Windows objects, or POSIX shm and futex counters.
#ifdef _WIN32
#include "win_objects.h"
#else
#include "posix_objects.h"
#endif
