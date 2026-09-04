#pragma once

#include "../common/PlatformBackend.hpp"

namespace btd4 {

class PSPBackend : public PlatformBackend {
public:
    std::string name() const override { return "PSP"; }
    std::string description() const override { return "PlayStation Portable (PSPSDK / PSP GU 480x272 / EBOOT.PBP)"; }

    bool isAvailable() const override;
    BuildResult configure() override;
    BuildResult build() override;
    BuildResult package() override;
};

} // namespace btd4
