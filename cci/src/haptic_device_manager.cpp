//
// Created by brant_contactci on 9/24/2024.
//

#include "cci/haptic_device_manager.h"

using namespace contactci;

class DeviceManager::Implementation {
public:
    Implementation() = delete;
    explicit Implementation(PipeChannel &channel);
    ~Implementation() = default;
    //getFirmware
    PipeChannel &channel;
};

class HapticDeviceManager::Implementation {
public:
    Implementation() = delete;
    explicit Implementation(PipeChannel &channel);
    ~Implementation() = default;
    [[nodiscard]] std::vector<CachedWaveform> get_cached_waveforms(const DeviceDescription &device,
                                                                   bool terse = false) const;
    PipeChannel &channel;
};

class MutableHapticDeviceManager::Implementation {
public:
    Implementation() = delete;
    ~Implementation() = default;
    explicit Implementation(PipeChannel &channel);
    void delete_waveform(const DeviceDescription &device, uint32_t index) const;
    WaveformTransferResponse transfer_waveform(const DeviceDescription &device, uint32_t sample_rate, uint8_t modifiers,
                                               std::vector<float> &samples,
                                               CachedWaveform &cache_out,
                                               const std::optional<std::string> &descriptor = std::nullopt) const;
    PipeChannel &channel;
};


DeviceManager::Implementation::Implementation(PipeChannel &channel): channel(channel) {
}

HapticDeviceManager::Implementation::Implementation(PipeChannel &channel): channel(channel) {
}

std::vector<CachedWaveform> HapticDeviceManager::Implementation::get_cached_waveforms(
    const DeviceDescription &device, bool terse) const {
    const auto table = channel.get_wav_table(device.get_serial_number(), terse);
    std::vector<CachedWaveform> res;
    for (const auto &wav: table.wavs()) {
        CachedWaveform wac;
        wac.index = wav.index();
        wac.length = wav.length();
        if (wav.has_identifier())
            wac.identifier = wav.identifier();
        if (wav.has_frequency())
            wac.sampleRate = wav.frequency();
        res.push_back(wac);
    }
    return res;
}

MutableHapticDeviceManager::Implementation::Implementation(PipeChannel &channel): channel(channel) {
}

void MutableHapticDeviceManager::Implementation::delete_waveform(const DeviceDescription &device,
                                                                 uint32_t index) const {
    channel.delete_waveform(device.get_serial_number(), index);
}

WaveformTransferResponse MutableHapticDeviceManager::Implementation::transfer_waveform(
    const DeviceDescription &device, uint32_t sample_rate, uint8_t modifiers, std::vector<float> &samples,
    CachedWaveform &cache_out, const std::optional<std::string> &descriptor) const {
    auto response = channel.upload_waveform(device.get_serial_number(), sample_rate, modifiers, samples, descriptor);
    cache_out = CachedWaveform{
        .index = response.index(),
        .identifier = descriptor,
        .sampleRate = sample_rate,
        .length = static_cast<uint32_t>(samples.size()),
    };

    switch (response.response()) {
        case wav_okay:
            return okay;
        case wav_noSpace:
            return noSpace;
        case wav_idxConflict:
            return indexConflict;
        case wav_invalid:
        default:
            return error;
    }
}

DeviceManager::DeviceManager() = default;

DeviceManager::~DeviceManager() = default;

DeviceManager::DeviceManager(PipeChannel &channel): implementation(std::make_unique<DeviceManager::Implementation>(channel)) {
}

DeviceManager::DeviceManager(const DeviceManager &other): implementation(std::make_unique<DeviceManager::Implementation>(*other.implementation)) {
}


HapticDeviceManager::HapticDeviceManager() = default;

HapticDeviceManager::~HapticDeviceManager() = default;

HapticDeviceManager::HapticDeviceManager(const DeviceManager &other): DeviceManager(other),
                                                                      implementation(std::make_unique<HapticDeviceManager::Implementation>(
                                                                          DeviceManager::implementation->channel)) {
}

std::vector<CachedWaveform> HapticDeviceManager::get_cached_waveforms(const DeviceDescription &device, bool terse) const {
    return implementation->get_cached_waveforms(device, terse);
}

MutableHapticDeviceManager::MutableHapticDeviceManager() = default;

MutableHapticDeviceManager::~MutableHapticDeviceManager() = default;

MutableHapticDeviceManager::MutableHapticDeviceManager(const DeviceManager &other): HapticDeviceManager(other),
                                                                                    implementation(std::make_unique<MutableHapticDeviceManager::Implementation>(
                                                                                        DeviceManager::implementation->channel)) {
}

void MutableHapticDeviceManager::delete_waveform(const DeviceDescription &device, uint32_t index) const {
    implementation->delete_waveform(device, index);
}

void MutableHapticDeviceManager::delete_all_waveform(const DeviceDescription &device) const {
    delete_waveform(device, UINT32_MAX);
}

WaveformTransferResponse MutableHapticDeviceManager::transfer_waveform(
    const DeviceDescription &device, uint32_t sample_rate, uint8_t modifiers, std::vector<float> &samples, CachedWaveform &cache_out,
    const std::optional<std::string> &descriptor) const {
    return implementation->transfer_waveform(device, sample_rate, modifiers, samples, cache_out, descriptor);
}
