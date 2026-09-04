#pragma once

#include <string_view>

namespace btd4 {

// Platform-independent audio service. Sound identifiers refer to entries in
// the internal asset manifest, never directly to platform-specific resources.
class IAudio {
public:
    virtual ~IAudio() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    virtual void playSound(std::string_view soundId, float gain = 1.0f) = 0;
    virtual void stopAllSounds() = 0;

    virtual void playMusic(std::string_view musicId, bool loop = true) = 0;
    virtual void stopMusic() = 0;

    virtual void setMasterVolume(float volume) = 0;
    virtual float masterVolume() const = 0;
    virtual void setMusicVolume(float volume) = 0;
    virtual float musicVolume() const = 0;
};

} // namespace btd4
