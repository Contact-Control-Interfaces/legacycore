//
// Created by john_contactci on 4/16/2024.
//

#pragma once

#include <string>

#include "platform.h"

namespace contactci {
    class NamedPipe {
    public:
        explicit NamedPipe(const std::string &name);
        NamedPipe(NamedPipe &&other) noexcept;
        ~NamedPipe();

        NativeHandle native_handle() const;

    private:
        NativeHandle pipe;
    };
}
