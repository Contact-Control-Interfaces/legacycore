//
// Created by brant_contactci on 9/24/2024.
//

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "cached_waveform.h"
#include <ccic/lib_defs.h>
#include "device.h"
#include "pipe_channel.h"

namespace contactci {
    class CCI_API_CLASS(DeviceManager) {
    public:
        explicit DeviceManager(PipeChannel &channel);
        DeviceManager(const DeviceManager &other);
        DeviceManager();
        virtual ~DeviceManager();
        //todo
        //const std::map<int, std::string> &get_firmware();
    protected:
        class Implementation;
        std::unique_ptr<DeviceManager::Implementation> implementation;
    };

    class CCI_API_CLASS(HapticDeviceManager) : public DeviceManager {
    public:
        HapticDeviceManager();
        virtual ~HapticDeviceManager();
        explicit HapticDeviceManager(const DeviceManager &other);
        std::vector<CachedWaveform> get_cached_waveforms(const DeviceDescription &device, bool terse = false) const;

    protected:
        class Implementation;
        std::unique_ptr<HapticDeviceManager::Implementation> implementation;
    };

    class CCI_API_CLASS(MutableHapticDeviceManager) : public HapticDeviceManager {
    public:
        MutableHapticDeviceManager();
        ~MutableHapticDeviceManager();
        explicit MutableHapticDeviceManager(const DeviceManager &other);
        void delete_waveform(const DeviceDescription &device, uint32_t index) const;
        void delete_all_waveform(const DeviceDescription &device) const;
        WaveformTransferResponse transfer_waveform(const DeviceDescription &device, uint32_t sample_rate, uint8_t modifiers,
                                               std::vector<float> &samples,
                                               CachedWaveform &cache_out,
                                               const std::optional<std::string> &descriptor = std::nullopt) const;

    protected:
        class Implementation;
        std::unique_ptr<MutableHapticDeviceManager::Implementation> implementation;
    };
}
