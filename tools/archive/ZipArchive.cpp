#include "ZipArchive.hpp"
#include "../swf/Inflate.hpp"

#include <algorithm>
#include <fstream>

namespace btd4::tools {

namespace {

uint16_t readU16(const std::vector<uint8_t>& bytes, size_t offset) {
    return static_cast<uint16_t>(bytes[offset]) |
           (static_cast<uint16_t>(bytes[offset + 1]) << 8);
}

uint32_t readU32(const std::vector<uint8_t>& bytes, size_t offset) {
    return static_cast<uint32_t>(bytes[offset]) |
           (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

bool hasBytes(const std::vector<uint8_t>& bytes, size_t offset, size_t count) {
    return offset <= bytes.size() && count <= bytes.size() - offset;
}

constexpr uint32_t kEndOfCentralDirectory = 0x06054b50;
constexpr uint32_t kCentralDirectoryEntry = 0x02014b50;
constexpr uint32_t kLocalFileHeader = 0x04034b50;
constexpr size_t kMaxArchiveSize = 256 * 1024 * 1024;
constexpr size_t kMaxEntrySize = 128 * 1024 * 1024;

} // namespace

bool ZipArchive::open(const std::string& path, std::string& error) {
    m_bytes.clear();
    m_entries.clear();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        error = "Unable to open archive: " + path;
        return false;
    }

    const std::streamoff size = file.tellg();
    if (size <= 0 || size > static_cast<std::streamoff>(kMaxArchiveSize)) {
        error = "Archive is empty or exceeds the 256 MiB safety limit.";
        return false;
    }

    m_bytes.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(m_bytes.data()), size);
    if (!file) {
        error = "Failed to read archive bytes.";
        m_bytes.clear();
        return false;
    }

    const size_t searchStart = m_bytes.size() > 65557 ? m_bytes.size() - 65557 : 0;
    size_t eocd = std::string::npos;
    for (size_t i = m_bytes.size(); i-- > searchStart;) {
        if (hasBytes(m_bytes, i, 4) && readU32(m_bytes, i) == kEndOfCentralDirectory) {
            eocd = i;
            break;
        }
    }
    if (eocd == std::string::npos || !hasBytes(m_bytes, eocd, 22)) {
        error = "ZIP end-of-central-directory record not found.";
        return false;
    }

    const uint16_t entryCount = readU16(m_bytes, eocd + 10);
    const uint32_t directorySize = readU32(m_bytes, eocd + 12);
    const uint32_t directoryOffset = readU32(m_bytes, eocd + 16);
    if (!hasBytes(m_bytes, directoryOffset, directorySize)) {
        error = "ZIP central directory is outside the archive.";
        return false;
    }

    size_t cursor = directoryOffset;
    for (uint16_t i = 0; i < entryCount; ++i) {
        if (!hasBytes(m_bytes, cursor, 46) || readU32(m_bytes, cursor) != kCentralDirectoryEntry) {
            error = "Invalid ZIP central directory entry.";
            m_entries.clear();
            return false;
        }

        const uint16_t method = readU16(m_bytes, cursor + 10);
        const uint32_t compressedSize = readU32(m_bytes, cursor + 20);
        const uint32_t uncompressedSize = readU32(m_bytes, cursor + 24);
        const uint16_t nameLength = readU16(m_bytes, cursor + 28);
        const uint16_t extraLength = readU16(m_bytes, cursor + 30);
        const uint16_t commentLength = readU16(m_bytes, cursor + 32);
        const uint32_t localOffset = readU32(m_bytes, cursor + 42);

        const size_t recordSize = 46ULL + nameLength + extraLength + commentLength;
        if (!hasBytes(m_bytes, cursor, recordSize)) {
            error = "Truncated ZIP central directory entry.";
            m_entries.clear();
            return false;
        }

        ZipEntry entry;
        entry.name.assign(reinterpret_cast<const char*>(m_bytes.data() + cursor + 46), nameLength);
        entry.compressionMethod = method;
        entry.compressedSize = compressedSize;
        entry.uncompressedSize = uncompressedSize;
        entry.localHeaderOffset = localOffset;
        m_entries.push_back(std::move(entry));
        cursor += recordSize;
    }

    return true;
}

bool ZipArchive::contains(const std::string& name) const {
    return std::any_of(m_entries.begin(), m_entries.end(), [&](const ZipEntry& entry) {
        return entry.name == name;
    });
}

bool ZipArchive::readEntry(const std::string& name, std::vector<uint8_t>& data, std::string& error) const {
    auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const ZipEntry& entry) {
        return entry.name == name;
    });
    if (it == m_entries.end()) {
        error = "ZIP entry not found: " + name;
        return false;
    }
    if (it->uncompressedSize > kMaxEntrySize || it->compressedSize > m_bytes.size()) {
        error = "ZIP entry exceeds the 128 MiB safety limit: " + name;
        return false;
    }

    const size_t local = it->localHeaderOffset;
    if (!hasBytes(m_bytes, local, 30) || readU32(m_bytes, local) != kLocalFileHeader) {
        error = "Invalid ZIP local header for: " + name;
        return false;
    }
    const uint16_t nameLength = readU16(m_bytes, local + 26);
    const uint16_t extraLength = readU16(m_bytes, local + 28);
    const size_t dataOffset = local + 30ULL + nameLength + extraLength;
    if (!hasBytes(m_bytes, dataOffset, it->compressedSize)) {
        error = "ZIP entry data is truncated: " + name;
        return false;
    }

    const uint8_t* compressed = m_bytes.data() + dataOffset;
    if (it->compressionMethod == 0) {
        data.assign(compressed, compressed + it->compressedSize);
        if (data.size() != it->uncompressedSize) {
            error = "Stored ZIP entry size mismatch: " + name;
            data.clear();
            return false;
        }
        return true;
    }

    if (it->compressionMethod == 8) {
        if (!swf::Inflate::decompressDeflate(compressed, it->compressedSize, data, it->uncompressedSize)) {
            error = "DEFLATE decompression failed for: " + name;
            data.clear();
            return false;
        }
        if (data.size() != it->uncompressedSize) {
            error = "DEFLATE output size mismatch for: " + name;
            data.clear();
            return false;
        }
        return true;
    }

    error = "Unsupported ZIP compression method " + std::to_string(it->compressionMethod) + " for: " + name;
    return false;
}

} // namespace btd4::tools
