#include "SDLAudio.hpp"

#include "../../engine/assets/AssetManager.hpp"
#include <SDL.h>
#include <algorithm>
#include <cmath>

namespace btd4 {

SDLAudio::~SDLAudio() {
    shutdown();
}

bool SDLAudio::initialize() {
    if (m_initialized) return true;
    if (Mix_Init(MIX_INIT_MP3) & MIX_INIT_MP3) {
        // MP3 support is supplied by the vendored minimp3 decoder in CI.
    } else {
        Mix_Init(0);
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        Mix_Quit();
        return false;
    }

    Mix_AllocateChannels(16);
    m_initialized = true;
    Mix_VolumeMusic(static_cast<int>(std::lround(m_masterVolume * m_musicVolume * MIX_MAX_VOLUME)));
    return true;
}

void SDLAudio::shutdown() {
    if (!m_initialized) return;

    stopAllSounds();
    stopMusic();

    for (auto& [id, chunk] : m_sounds) {
        (void)id;
        if (chunk) Mix_FreeChunk(chunk);
    }
    m_sounds.clear();

    Mix_CloseAudio();
    Mix_Quit();
    m_initialized = false;
}

bool SDLAudio::isInitialized() const {
    return m_initialized;
}

void SDLAudio::playSound(std::string_view soundId, float gain) {
    if (!m_initialized || soundId.empty()) return;

    const std::string id(soundId);
    Mix_Chunk*& chunk = m_sounds[id];
    if (!chunk) {
        const std::string path = AssetManager::instance().resolveAudioPath(id);
        if (path.empty()) {
            m_sounds.erase(id);
            return;
        }
        chunk = Mix_LoadWAV(path.c_str());
        if (!chunk) {
            m_sounds.erase(id);
            return;
        }
    }

    const float volume = std::clamp(gain, 0.0f, 1.0f) * m_masterVolume;
    Mix_VolumeChunk(chunk, static_cast<int>(std::lround(volume * MIX_MAX_VOLUME)));
    Mix_PlayChannel(-1, chunk, 0);
}

void SDLAudio::stopAllSounds() {
    if (m_initialized) Mix_HaltChannel(-1);
}

void SDLAudio::playMusic(std::string_view musicId, bool loop) {
    if (!m_initialized || musicId.empty()) return;

    const std::string path = AssetManager::instance().resolveAudioPath(std::string(musicId));
    if (path.empty()) return;

    if (m_music) {
        Mix_FreeMusic(m_music);
        m_music = nullptr;
    }

    m_music = Mix_LoadMUS(path.c_str());
    if (!m_music) return;

    Mix_VolumeMusic(static_cast<int>(std::lround(m_masterVolume * m_musicVolume * MIX_MAX_VOLUME)));
    Mix_PlayMusic(m_music, loop ? -1 : 1);
}

void SDLAudio::stopMusic() {
    if (!m_music) return;
    Mix_HaltMusic();
    Mix_FreeMusic(m_music);
    m_music = nullptr;
}

void SDLAudio::setMasterVolume(float volume) {
    m_masterVolume = clampVolume(volume);
    if (m_initialized)
        Mix_VolumeMusic(static_cast<int>(std::lround(m_masterVolume * m_musicVolume * MIX_MAX_VOLUME)));
}

float SDLAudio::masterVolume() const {
    return m_masterVolume;
}

void SDLAudio::setMusicVolume(float volume) {
    m_musicVolume = clampVolume(volume);
    if (m_initialized)
        Mix_VolumeMusic(static_cast<int>(std::lround(m_masterVolume * m_musicVolume * MIX_MAX_VOLUME)));
}

float SDLAudio::musicVolume() const {
    return m_musicVolume;
}

float SDLAudio::clampVolume(float volume) {
    return std::clamp(volume, 0.0f, 1.0f);
}

} // namespace btd4
