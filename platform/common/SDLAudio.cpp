#include "SDLAudio.hpp"

#include "../../engine/assets/AssetManager.hpp"
#include <algorithm>

namespace btd4 {

SDLAudio::~SDLAudio() {
    shutdown();
}

bool SDLAudio::initialize() {
    if (m_initialized) return true;

    if (!MIX_Init()) {
        return false;
    }

    m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!m_mixer) {
        MIX_Quit();
        return false;
    }

    if (!MIX_SetMixerGain(m_mixer, m_masterVolume)) {
        shutdown();
        return false;
    }

    m_soundTracks.reserve(16);
    for (int i = 0; i < 16; ++i) {
        MIX_Track* track = MIX_CreateTrack(m_mixer);
        if (!track) {
            shutdown();
            return false;
        }
        m_soundTracks.push_back(track);
    }

    m_musicTrack = MIX_CreateTrack(m_mixer);
    if (!m_musicTrack) {
        shutdown();
        return false;
    }

    m_initialized = true;
    return true;
}

void SDLAudio::shutdown() {
    if (!m_mixer) {
        MIX_Quit();
        m_initialized = false;
        return;
    }

    stopAllSounds();
    stopMusic();

    for (MIX_Track* track : m_soundTracks) {
        if (track) MIX_DestroyTrack(track);
    }
    m_soundTracks.clear();

    if (m_musicTrack) {
        MIX_DestroyTrack(m_musicTrack);
        m_musicTrack = nullptr;
    }

    for (auto& [id, audio] : m_sounds) {
        (void)id;
        if (audio) MIX_DestroyAudio(audio);
    }
    m_sounds.clear();

    MIX_DestroyMixer(m_mixer);
    m_mixer = nullptr;

    m_initialized = false;
    m_nextSoundTrack = 0;
    MIX_Quit();
}

bool SDLAudio::isInitialized() const {
    return m_initialized;
}

void SDLAudio::playSound(std::string_view soundId, float gain) {
    if (!m_initialized || !m_mixer || soundId.empty() || m_soundTracks.empty()) return;

    const std::string id(soundId);
    MIX_Audio*& audio = m_sounds[id];
    if (!audio) {
        const std::string path = AssetManager::instance().resolveAudioPath(id);
        if (path.empty()) {
            m_sounds.erase(id);
            return;
        }
        audio = MIX_LoadAudio(m_mixer, path.c_str(), true);
        if (!audio) {
            m_sounds.erase(id);
            return;
        }
    }

    MIX_Track* track = m_soundTracks[m_nextSoundTrack];
    m_nextSoundTrack = (m_nextSoundTrack + 1) % m_soundTracks.size();
    MIX_SetTrackAudio(track, audio);
    MIX_SetTrackGain(track, std::clamp(gain, 0.0f, 1.0f) * m_masterVolume);
    MIX_SetTrackLoops(track, 0);
    MIX_PlayTrack(track, 0);
}

void SDLAudio::stopAllSounds() {
    for (MIX_Track* track : m_soundTracks) {
        if (track) MIX_StopTrack(track, 0);
    }
}

void SDLAudio::playMusic(std::string_view musicId, bool loop) {
    if (!m_initialized || !m_mixer || !m_musicTrack || musicId.empty()) return;

    const std::string path = AssetManager::instance().resolveAudioPath(std::string(musicId));
    if (path.empty()) return;

    stopMusic();

    m_music = MIX_LoadAudio(m_mixer, path.c_str(), false);
    if (!m_music) return;

    MIX_SetTrackAudio(m_musicTrack, m_music);
    MIX_SetTrackGain(m_musicTrack, m_masterVolume * m_musicVolume);
    MIX_SetTrackLoops(m_musicTrack, loop ? -1 : 0);
    if (!MIX_PlayTrack(m_musicTrack, 0)) {
        MIX_DestroyAudio(m_music);
        m_music = nullptr;
    }
}

void SDLAudio::stopMusic() {
    if (m_musicTrack) MIX_StopTrack(m_musicTrack, 0);
    if (m_music) {
        MIX_DestroyAudio(m_music);
        m_music = nullptr;
    }
}

void SDLAudio::setMasterVolume(float volume) {
    m_masterVolume = clampVolume(volume);
    if (m_mixer) MIX_SetMixerGain(m_mixer, m_masterVolume);
    if (m_musicTrack) MIX_SetTrackGain(m_musicTrack, m_masterVolume * m_musicVolume);
}

float SDLAudio::masterVolume() const {
    return m_masterVolume;
}

void SDLAudio::setMusicVolume(float volume) {
    m_musicVolume = clampVolume(volume);
    if (m_musicTrack) MIX_SetTrackGain(m_musicTrack, m_masterVolume * m_musicVolume);
}

float SDLAudio::musicVolume() const {
    return m_musicVolume;
}

float SDLAudio::clampVolume(float volume) {
    return std::clamp(volume, 0.0f, 1.0f);
}

} // namespace btd4
