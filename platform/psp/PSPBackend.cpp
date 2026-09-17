#include "PSPBackend.hpp"
#include <cstdlib>
#include <filesystem>
#include <string>

namespace btd4 {
namespace {

bool executableOnPath(const char* executable) {
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv || !*pathEnv || !executable || !*executable) return false;

#ifdef _WIN32
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif

    std::string pathList(pathEnv);
    size_t start = 0;
    while (start <= pathList.size()) {
        const size_t end = pathList.find(separator, start);
        const std::string entry = pathList.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!entry.empty()) {
            std::filesystem::path candidate = std::filesystem::path(entry) / executable;
            std::error_code ec;
            if (std::filesystem::is_regular_file(candidate, ec)) return true;
#ifdef _WIN32
            candidate += ".exe";
            if (std::filesystem::is_regular_file(candidate, ec)) return true;
#endif
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return false;
}

} // namespace

bool PSPBackend::isAvailable() const {
    static const bool available = [] {
        const char* pspdev = std::getenv("PSPDEV");
        if (pspdev != nullptr && pspdev[0] != '\0') return true;
        return executableOnPath("psp-gcc");
    }();
    return available;
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

BuildResult PSPBackend::package(const std::string&) {
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
