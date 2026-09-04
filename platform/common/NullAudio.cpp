#include "NullAudio.hpp"

#include <algorithm>

namespace btd4 {

bool NullAudio::initialize() {
    m_initialized = true;
    return true;
}

void NullAudio::shutdown() {
    m_initialized = false;
}

bool NullAudio::isInitialized() const {
    return m_initialized;
}

void NullAudio::playSound(std::string_view soundId, float gain) {
    (void)soundId;
    (void)gain;
}

void NullAudio::stopAllSounds() {
}

void NullAudio::playMusic(std::string_view musicId, bool loop) {
    (void)musicId;
    (void)loop;
}

void NullAudio::stopMusic() {
}

void NullAudio::setMasterVolume(float volume) {
    m_masterVolume = clampVolume(volume);
}

float NullAudio::masterVolume() const {
    return m_masterVolume;
}

void NullAudio::setMusicVolume(float volume) {
    m_musicVolume = clampVolume(volume);
}

float NullAudio::musicVolume() const {
    return m_musicVolume;
}

float NullAudio::clampVolume(float volume) {
    return std::clamp(volume, 0.0f, 1.0f);
}

} // namespace btd4
