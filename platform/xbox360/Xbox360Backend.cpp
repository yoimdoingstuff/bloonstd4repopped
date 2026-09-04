#include "Xbox360Backend.hpp"
#include <cstdlib>

namespace btd4 {

bool Xbox360Backend::isAvailable() const {
    const char* xdk = std::getenv("XEDK");
    return (xdk != nullptr && xdk[0] != '\0');
}

BuildResult Xbox360Backend::configure() {
    BuildResult res;
    if (!isAvailable()) {
        res.message = "Official Xbox 360 development environment not detected. Install/configure legitimate toolchain.";
        return res;
    }
    res.message = "Xbox 360 build configuration is not implemented; no project files were generated.";
    return res;
}

BuildResult Xbox360Backend::build() {
    BuildResult res;
    res.success = false;
    res.message = "Xbox 360 build target is scheduled for Phase 11 and is not yet implemented.";
    return res;
}

BuildResult Xbox360Backend::package() {
    BuildResult res;
    res.success = false;
    res.message = "Xbox 360 packaging is scheduled for Phase 11.";
    return res;
}

} // namespace btd4
