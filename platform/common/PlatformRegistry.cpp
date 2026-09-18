#include "PlatformRegistry.hpp"
#include "../linux/LinuxPlatform.hpp"
#include "../windows/WindowsPlatform.hpp"
#include "../psp/PSPBackend.hpp"
#include "../xbox360/Xbox360Backend.hpp"
#include "../future/FuturePlatformBackend.hpp"

namespace btd4 {

PlatformRegistry& PlatformRegistry::instance() {
    static PlatformRegistry s_instance;
    return s_instance;
}

PlatformRegistry::PlatformRegistry() {
    // Register default platform backends
    registerBackend(std::make_unique<LinuxPlatform>());
    registerBackend(std::make_unique<WindowsPlatform>());
    registerBackend(std::make_unique<PSPBackend>());
    registerBackend(std::make_unique<Xbox360Backend>());
    registerBackend(std::make_unique<FuturePlatformBackend>(
        "PlayStation Vita",
        "PlayStation Vita Future Target / VitaSDK / Native Package",
        "VITASDK"));
    registerBackend(std::make_unique<FuturePlatformBackend>(
        "Android",
        "Android Future Target / Android 4.0+ Compatible Build (planned)",
        "ANDROID_NDK_HOME"));
    registerBackend(std::make_unique<FuturePlatformBackend>(
        "iOS",
        "iOS Future Target / Jailbroken and legacy iOS 9-or-earlier build path (planned)",
        "IOS_SDK"));
}

void PlatformRegistry::registerBackend(std::unique_ptr<PlatformBackend> backend) {
    if (backend) {
        m_backends.push_back(std::move(backend));
    }
}

const std::vector<std::unique_ptr<PlatformBackend>>& PlatformRegistry::backends() const {
    return m_backends;
}

PlatformBackend* PlatformRegistry::findBackend(const std::string& name) const {
    for (const auto& backend : m_backends) {
        if (backend->name() == name) {
            return backend.get();
        }
    }
    return nullptr;
}

} // namespace btd4
