#pragma once

#include <string>
#include <vector>

namespace btd4 {

struct BuildResult {
    bool success{false};
    std::string message;
    std::vector<std::string> outputLogs;
};

class PlatformBackend {
public:
    virtual ~PlatformBackend() = default;

    virtual std::string name() const = 0;
    virtual std::string description() const = 0;

    // Checks if the platform's compiler/toolchain is installed & available
    virtual bool isAvailable() const = 0;

    // Build steps return success only after the backend performed and verified
    // the requested operation. Unsupported or unfinished steps must fail with
    // an actionable message.
    virtual BuildResult configure() = 0;
    virtual BuildResult build() = 0;
    // Package only the edition selected by the Game Builder. An empty edition
    // keeps compatibility with backends that do not yet package game data.
    virtual BuildResult package(const std::string& gameEdition = {}) = 0;
};

} // namespace btd4
