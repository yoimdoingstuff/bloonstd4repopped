#pragma once

#include "../../engine/audio/IAudio.hpp"
#include <SDL3_mixer/SDL_mixer.h>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace btd4 {

class SDLAudio final : public IAudio {
public:
    ~SDLAudio() override;

    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override;

    void playSound(std::string_view soundId, float gain = 1.0f) override;
    void stopAllSounds() override;

    void playMusic(std::string_view musicId, bool loop = true) override;
    void stopMusic() override;

    void setMasterVolume(float volume) override;
    float masterVolume() const override;
    void setMusicVolume(float volume) override;
    float musicVolume() const override;

private:
    static float clampVolume(float volume);

    bool m_initialized{false};
    float m_masterVolume{1.0f};
    float m_musicVolume{1.0f};

    MIX_Mixer* m_mixer{nullptr};
    MIX_Audio* m_music{nullptr};
    MIX_Track* m_musicTrack{nullptr};

    std::unordered_map<std::string, MIX_Audio*> m_sounds;
    std::vector<MIX_Track*> m_soundTracks;
    std::size_t m_nextSoundTrack{0};
};

} // namespace btd4
