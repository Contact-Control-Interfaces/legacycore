//
// Created by brant_contactci on 9/24/2024.
//

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "cached_waveform.h"
#include <ccic/lib_defs.h>

#include "pipe_channel.h"
#include "session.h"

namespace contactci {
    class CCI_API_CLASS(DeviceManager) {
        public:
        explicit DeviceManager(PipeChannel &channel);
        virtual ~DeviceManager();
        //todo
        //const std::map<int, std::string> &get_firmware();
        protected:
        PipeChannel &channel;
    };

    class CCI_API_CLASS(HapticDeviceManager) : public DeviceManager {
    public:
        explicit HapticDeviceManager(PipeChannel &channel);
        [[nodiscard]] std::vector<CachedWaveform> get_cached_waveforms(bool terse = false) const;
    };

    class CCI_API_CLASS(MutableHapticDeviceManager) : public HapticDeviceManager {
    public:
        explicit MutableHapticDeviceManager(PipeChannel &channel);
        void delete_waveform(uint32_t index) const;
        void delete_all_waveform() const;
        UploadClientWaveformResponseMessage transfer_waveform(uint32_t sample_rate, const std::vector<const float> &samples,
                                                   CachedWaveform &cache_out,
                                                   const std::optional<std::string>& descriptor = std::nullopt) const;
    };
}
