#pragma once

#include "PlatformBackend.hpp"
#include <memory>
#include <vector>

namespace btd4 {

class PlatformRegistry {
public:
    static PlatformRegistry& instance();

    void registerBackend(std::unique_ptr<PlatformBackend> backend);
    const std::vector<std::unique_ptr<PlatformBackend>>& backends() const;

    PlatformBackend* findBackend(const std::string& name) const;

private:
    PlatformRegistry();
    ~PlatformRegistry() = default;

    std::vector<std::unique_ptr<PlatformBackend>> m_backends;
};

} // namespace btd4
