#pragma once

#include "../../engine/audio/IAudio.hpp"
#include <SDL_mixer.h>
#include <string>
#include <unordered_map>

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
    Mix_Music* m_music{nullptr};
    std::unordered_map<std::string, Mix_Chunk*> m_sounds;
};

} // namespace btd4
