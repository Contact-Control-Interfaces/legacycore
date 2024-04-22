//
// Created by john_contactci on 4/19/2024.
//

#include "client.h"

using namespace contactci;

ClientDescription::ClientDescription(uint32_t process_id, std::string process_name)
    : process_id(process_id), process_name(std::move(process_name)) {}

uint32_t ClientDescription::get_process_id() const {
    return process_id;
}

const std::string &ClientDescription::get_process_name() const {
    return process_name;
}