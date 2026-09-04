#pragma once

#include "../../engine/audio/IAudio.hpp"

namespace btd4 {

// Deliberately silent backend used where native audio is unavailable. It keeps
// game code independent of the selected platform without pretending playback
// occurred.
class NullAudio final : public IAudio {
public:
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
};

} // namespace btd4
