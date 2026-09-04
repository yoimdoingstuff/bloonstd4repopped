#include "PlatformRegistry.hpp"
#include "../linux/LinuxPlatform.hpp"
#include "../windows/WindowsPlatform.hpp"
#include "../psp/PSPBackend.hpp"
#include "../xbox360/Xbox360Backend.hpp"

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
