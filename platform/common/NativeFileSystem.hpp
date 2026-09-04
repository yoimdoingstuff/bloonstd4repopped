#pragma once

#include "../../engine/core/IFileSystem.hpp"

namespace btd4 {

// Desktop implementation shared by the Windows and Linux platform backends.
class NativeFileSystem final : public IFileSystem {
public:
    bool readFile(const std::string& path, std::vector<uint8_t>& data) const override;
    bool writeFile(const std::string& path, const std::vector<uint8_t>& data) override;
    bool fileExists(const std::string& path) const override;
    bool createDirectories(const std::string& path) override;
    bool removeFile(const std::string& path) override;
    bool listFiles(const std::string& path, std::vector<std::string>& files) const override;
};

} // namespace btd4
