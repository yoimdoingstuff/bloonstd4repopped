#include "LinuxPlatform.hpp"
#include <cstdlib>

namespace btd4 {

bool LinuxPlatform::isAvailable() const {
#if defined(__linux__)
    // Check if g++ or clang++ is present in PATH
    int hasGcc = std::system("which g++ > /dev/null 2>&1");
    int hasClang = std::system("which clang++ > /dev/null 2>&1");
    int hasCmake = std::system("which cmake > /dev/null 2>&1");
    return ((hasGcc == 0 || hasClang == 0) && hasCmake == 0);
#else
    return false;
#endif
}

BuildResult LinuxPlatform::configure() {
    BuildResult res;
    if (!isAvailable()) {
        res.message = "Linux toolchain (g++/clang++ and cmake) not found.";
        return res;
    }
    res.message = "Linux build execution is not implemented yet; no CMake configuration was run.";
    res.outputLogs.push_back("The Game Builder build executor is required before Linux configuration can run.");
    return res;
}

BuildResult LinuxPlatform::build() {
    BuildResult res;
    if (!isAvailable()) {
        res.message = "Linux toolchain not available.";
        return res;
    }
    res.message = "Linux build execution is not implemented yet; no binaries were produced.";
    res.outputLogs.push_back("The Game Builder build executor is required before Linux builds can run.");
    return res;
}

BuildResult LinuxPlatform::package() {
    BuildResult res;
    res.message = "Linux packaging is not implemented yet; no package was produced.";
    res.outputLogs.push_back("The Game Builder packaging executor is required before Linux packaging can run.");
    return res;
}

} // namespace btd4
