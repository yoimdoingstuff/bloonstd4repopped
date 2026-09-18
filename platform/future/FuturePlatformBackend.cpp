#include "FuturePlatformBackend.hpp"
#include <cstdlib>

namespace btd4 {

FuturePlatformBackend::FuturePlatformBackend(std::string platformName,
                                             std::string platformDescription,
                                             std::string availabilityVariable)
    : m_name(std::move(platformName)),
      m_description(std::move(platformDescription)),
      m_availabilityVariable(std::move(availabilityVariable)) {}

bool FuturePlatformBackend::isAvailable() const {
    if (m_availabilityVariable.empty()) return false;
    const char* value = std::getenv(m_availabilityVariable.c_str());
    return value != nullptr && value[0] != '\0';
}

BuildResult FuturePlatformBackend::configure() {
    BuildResult result;
    if (!isAvailable()) {
        result.message = m_name + " development toolchain is not configured on this host.";
        return result;
    }
    result.message = m_name + " backend is registered, but configuration is not implemented yet.";
    return result;
}

BuildResult FuturePlatformBackend::build() {
    BuildResult result;
    result.message = m_name + " build target is registered as a future-platform skeleton and is not implemented yet.";
    return result;
}

BuildResult FuturePlatformBackend::package(const std::string&) {
    BuildResult result;
    result.message = m_name + " packaging is registered as a future-platform skeleton and is not implemented yet.";
    return result;
}

} // namespace btd4
