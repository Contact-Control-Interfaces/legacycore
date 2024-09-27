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
        enum OtaStatus {
            error,      //general unspecified error
            okay,       //success
            checksum,   //checksum mismatch
            version,    //supplied version older than current version
            noSpace     //no space on device for OTA files
        };
        explicit DeviceManager(PipeChannel &channel);
        DeviceManager(const DeviceManager &other);
        DeviceManager();
        virtual ~DeviceManager();
        //accepts a URI to the firmware package, not a simple file path
        OtaStatus SubmitOtaUpdate(const DeviceDescription &device, const std::string &uri, const std::optional<std::function<void(float)>> &progressCallback) const;

    protected:
        class Implementation;
        std::unique_ptr<DeviceManager::Implementation> implementation;
    };

    class CCI_API_CLASS(HapticDeviceManager) : public DeviceManager {
    public:
        HapticDeviceManager();
        virtual ~HapticDeviceManager();
        explicit HapticDeviceManager(const DeviceManager &other);
        std::vector<CachedWaveform> get_cached_waveforms(const std::string &serialNumber, bool terse = false) const;

    protected:
        class Implementation;
        std::unique_ptr<HapticDeviceManager::Implementation> implementation;
    };

    class CCI_API_CLASS(MutableHapticDeviceManager) : public HapticDeviceManager {
    public:
        MutableHapticDeviceManager();
        ~MutableHapticDeviceManager();
        explicit MutableHapticDeviceManager(const DeviceManager &other);
        void delete_waveform(const std::string &serialNumber, uint32_t index) const;
        void delete_all_waveform(const std::string &serialNumber) const;
        WaveformTransferResponse transfer_waveform(const std::string &serialNumber, uint32_t sample_rate, uint8_t modifiers,
                                               std::vector<float> &samples,
                                               CachedWaveform &cache_out,
                                               const std::optional<std::string> &descriptor = std::nullopt) const;

    protected:
        class Implementation;
        std::unique_ptr<MutableHapticDeviceManager::Implementation> implementation;
    };
}
