//
// Created by john_contactci on 3/18/2024.
//

#pragma once

#include <string>
#include <stdexcept>

#include <ccic/lib_defs.h>
#include <ccic/error.h>

namespace contactci {
    class CCI_API_CLASS(Exception) : std::runtime_error {
    public:
        explicit Exception(CciStatus errorCode);

        CciStatus get_error_code() const;

    private:
        CciStatus errorCode;
    };
}
