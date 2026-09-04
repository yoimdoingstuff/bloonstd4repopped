#include "PSPBackend.hpp"
#include <cstdlib>

namespace btd4 {

bool PSPBackend::isAvailable() const {
    const char* pspdev = std::getenv("PSPDEV");
    if (pspdev != nullptr && pspdev[0] != '\0') {
        return true;
    }

    int hasPspGcc = std::system("which psp-gcc > /dev/null 2>&1");
    return (hasPspGcc == 0);
}

BuildResult PSPBackend::configure() {
    BuildResult res;
    if (!isAvailable()) {
        res.message = "PSPDEV toolchain (psp-gcc) not detected. Ensure PSPDEV environment variable is configured.";
        return res;
    }
    res.message = "PSP build execution is not implemented yet; no CMake configuration was run.";
    res.outputLogs.push_back("The Game Builder build executor is required before PSP configuration can run.");
    return res;
}

BuildResult PSPBackend::build() {
    BuildResult res;
    if (!isAvailable()) {
        res.message = "PSP toolchain not available.";
        return res;
    }
    res.message = "PSP build execution is not implemented yet; no ELF was produced.";
    res.outputLogs.push_back("The Game Builder build executor is required before PSP builds can run.");
    return res;
}

BuildResult PSPBackend::package() {
    BuildResult res;
    if (!isAvailable()) {
        res.message = "PSP toolchain not available.";
        return res;
    }
    res.message = "PSP packaging is not implemented yet; no EBOOT.PBP was produced.";
    res.outputLogs.push_back("The Game Builder packaging executor is required before PSP packaging can run.");
    return res;
}

} // namespace btd4
