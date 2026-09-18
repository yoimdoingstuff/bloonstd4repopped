#pragma once

#include "../common/PlatformBackend.hpp"

namespace btd4 {

class FuturePlatformBackend final : public PlatformBackend {
public:
    FuturePlatformBackend(std::string platformName, std::string platformDescription,
                          std::string availabilityVariable = {});

    std::string name() const override { return m_name; }
    std::string description() const override { return m_description; }

    bool isAvailable() const override;
    BuildResult configure() override;
    BuildResult build() override;
    BuildResult package(const std::string& gameEdition = {}) override;

private:
    std::string m_name;
    std::string m_description;
    std::string m_availabilityVariable;
};

} // namespace btd4
