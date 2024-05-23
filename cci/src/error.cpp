//
// Created by john_contactci on 3/18/2024.
//

#include "cci/error.h"

#include <windows.h>

contactci::Exception::Exception(CciStatus errorCode)
    : std::runtime_error(get_error_string(errorCode)), errorCode(errorCode) {}

CciStatus contactci::Exception::get_error_code() const {
    return errorCode;
}