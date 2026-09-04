#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace btd4 {

// Platform storage interface. Paths are UTF-8 and relative paths are resolved
// by the active backend; game code must not call OS filesystem APIs directly.
class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    virtual bool readFile(const std::string& path, std::vector<uint8_t>& data) const = 0;
    virtual bool writeFile(const std::string& path, const std::vector<uint8_t>& data) = 0;
    virtual bool fileExists(const std::string& path) const = 0;
    virtual bool createDirectories(const std::string& path) = 0;
    virtual bool removeFile(const std::string& path) = 0;
    virtual bool listFiles(const std::string& path, std::vector<std::string>& files) const = 0;
};

} // namespace btd4
