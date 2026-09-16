#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace btd4::tools {

struct ZipEntry {
    std::string name;
    uint16_t compressionMethod{0};
    uint32_t compressedSize{0};
    uint32_t uncompressedSize{0};
    uint32_t localHeaderOffset{0};
};

class ZipArchive {
public:
    bool open(const std::string& path, std::string& error);
    bool contains(const std::string& name) const;
    bool readEntry(const std::string& name, std::vector<uint8_t>& data, std::string& error) const;
    const std::vector<ZipEntry>& entries() const { return m_entries; }

private:
    std::vector<uint8_t> m_bytes;
    std::vector<ZipEntry> m_entries;
};

} // namespace btd4::tools
