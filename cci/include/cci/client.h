//
// Created by john_contactci on 4/19/2024.
//

#pragma once

#include <ccic/lib_defs.h>

#include <string>
#include <cstdint>

namespace contactci {
    class CCI_API_CLASS(ClientDescription) {
    public:
        ClientDescription(uint32_t process_id, std::string process_name);

        uint32_t get_process_id() const;
        const std::string &get_process_name() const;

    private:
        uint32_t process_id;
        std::string process_name;
    };
}