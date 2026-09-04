#pragma once

#include "../common/PlatformBackend.hpp"

namespace btd4 {

class WindowsPlatform : public PlatformBackend {
public:
    std::string name() const override { return "Windows"; }
    std::string description() const override { return "Native 64-bit Windows Desktop (MSVC / MinGW / SDL2)"; }

    bool isAvailable() const override;
    BuildResult configure() override;
    BuildResult build() override;
    BuildResult package() override;
};

} // namespace btd4
