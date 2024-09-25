//
// Created by brant_contactci on 9/24/2024.
//

#include "cci/haptic_device_manager.h"

#include "cci/pipe_channel.h"

using namespace contactci;


DeviceManager::DeviceManager(PipeChannel &channel) : channel(channel){
}

DeviceManager::~DeviceManager() = default;

DeviceManager::DeviceManager(const DeviceManager &other): channel(other.channel) {
}

HapticDeviceManager::HapticDeviceManager(const DeviceManager &base): DeviceManager(base) {
}

std::vector<CachedWaveform> HapticDeviceManager::get_cached_waveforms(const bool terse) const {
    const auto table = channel.get_wav_table(terse);
    std::vector<CachedWaveform> res;
    for(const auto& wav : table.wavs()) {
        CachedWaveform wac;
        wac.index = wav.index();
        wac.length = wav.length();
        if(wav.has_identifier())
            wac.identifier = wav.identifier();
        if(wav.has_frequency())
            wac.sampleRate = wav.frequency();
        res.push_back(wac);
    }
    return res;
}

MutableHapticDeviceManager::MutableHapticDeviceManager(const DeviceManager &base): HapticDeviceManager(base) {
}

void MutableHapticDeviceManager::delete_waveform(uint32_t index) const {
    channel.delete_waveform(index);
}

void MutableHapticDeviceManager::delete_all_waveform() const {
    delete_waveform(UINT32_MAX);
}

UploadClientWaveformResponseMessage MutableHapticDeviceManager::transfer_waveform(uint32_t sample_rate, std::vector<float> &samples,
    CachedWaveform &cache_out, const std::optional<std::string>& descriptor) const {
    auto response = channel.upload_waveform(sample_rate, samples, descriptor);
    cache_out = CachedWaveform{
        .index = response.index(),
        .sampleRate = sample_rate,
        .length = static_cast<uint32_t>(samples.size()),
        .identifier = descriptor.value_or("")
    };
    return response;
}
