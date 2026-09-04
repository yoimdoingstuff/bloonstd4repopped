#pragma once

#include "../common/PlatformBackend.hpp"

namespace btd4 {

class Xbox360Backend : public PlatformBackend {
public:
    std::string name() const override { return "Xbox 360"; }
    std::string description() const override { return "Xbox 360 Console Backend (Future Target / XDK / Title Package)"; }

    bool isAvailable() const override;
    BuildResult configure() override;
    BuildResult build() override;
    BuildResult package() override;
};

} // namespace btd4
