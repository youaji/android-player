#include "AudioDevice.h"

AudioDevice::AudioDevice() {}

AudioDevice::~AudioDevice() {}

int AudioDevice::open(const AudioDeviceSpec *desired, AudioDeviceSpec *obtained) {
    return 0;
}

void AudioDevice::start() {}

void AudioDevice::stop() {}

void AudioDevice::pause() {}

void AudioDevice::resume() {}

void AudioDevice::flush() {}

void AudioDevice::setVolume(float volume) {}

float AudioDevice::getVolume() {
    return 0;
}

void AudioDevice::setStereoVolume(float left_volume, float right_volume) {}

void AudioDevice::run() {}