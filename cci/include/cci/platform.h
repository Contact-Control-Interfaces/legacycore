//
// Created by zach_contactci on 10/13/2026.
//

#pragma once

namespace contactci {
    // An OS handle, without pulling OS headers into shared headers: HANDLE (void *) on Windows, a file descriptor on POSIX
#ifdef _WIN32
    using NativeHandle = void *;
#else
    using NativeHandle = int;
#endif
}
