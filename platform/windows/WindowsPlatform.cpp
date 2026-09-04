#include "WindowsPlatform.hpp"
#include <cstdlib>

namespace btd4 {

bool WindowsPlatform::isAvailable() const {
#if defined(_WIN32)
    return true;
#else
    // Check if MinGW cross-compiler is available on non-Windows host
    int hasMingw = std::system("which x86_64-w64-mingw32-g++ > /dev/null 2>&1");
    return (hasMingw == 0);
#endif
}

BuildResult WindowsPlatform::configure() {
    BuildResult res;
    if (!isAvailable()) {
        res.message = "Windows toolchain (MSVC or MinGW) not detected.";
        return res;
    }
    res.message = "Windows build execution is not implemented yet; no CMake configuration was run.";
    res.outputLogs.push_back("The Game Builder build executor is required before Windows configuration can run.");
    return res;
}

BuildResult WindowsPlatform::build() {
    BuildResult res;
    if (!isAvailable()) {
        res.message = "Windows toolchain not available.";
        return res;
    }
    res.message = "Windows build execution is not implemented yet; no binaries were produced.";
    res.outputLogs.push_back("The Game Builder build executor is required before Windows builds can run.");
    return res;
}

BuildResult WindowsPlatform::package() {
    BuildResult res;
    res.message = "Windows packaging is not implemented yet; no archive or installer was produced.";
    res.outputLogs.push_back("The Game Builder packaging executor is required before Windows packaging can run.");
    return res;
}

} // namespace btd4
