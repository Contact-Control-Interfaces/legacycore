//
// Created by brant_contactci on 9/24/2024.
//

#pragma once

#include <string>

struct CachedWaveform{
    uint32_t index;
    std::optional<std::string> identifier;
    std::optional<uint32_t> sampleRate;
    uint32_t length;
};

enum WaveformTransferResponse {
    error,
    indexConflict,
    noSpace,
    okay
};