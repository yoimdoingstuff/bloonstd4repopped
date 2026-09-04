#pragma once

#include "../common/PlatformBackend.hpp"

namespace btd4 {

class LinuxPlatform : public PlatformBackend {
public:
    std::string name() const override { return "Linux"; }
    std::string description() const override { return "Native 64-bit Linux Desktop (SDL2 / OpenGL / AppImage)"; }

    bool isAvailable() const override;
    BuildResult configure() override;
    BuildResult build() override;
    BuildResult package() override;
};

} // namespace btd4
