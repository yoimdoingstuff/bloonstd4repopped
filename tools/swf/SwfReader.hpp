#pragma once

#include "SwfTypes.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

namespace btd4::swf {

class SwfReader {
public:
    SwfReader(const uint8_t* data, size_t size);
    SwfReader(const std::vector<uint8_t>& buffer);

    size_t position() const { return m_pos; }
    size_t size() const { return m_size; }
    size_t remaining() const { return (m_pos < m_size) ? (m_size - m_pos) : 0; }
    bool isEof() const { return m_pos >= m_size; }

    void seek(size_t pos);
    void skip(size_t bytes);

    // Byte-level primitives
    uint8_t readUI8();
    uint16_t readUI16();
    uint32_t readUI32();
    int8_t readSI8();
    int16_t readSI16();
    int32_t readSI32();
    float readFixed8_8();

    // Variable length encoded integer (used in ABC blocks)
    uint32_t readU30();
    uint32_t readU32();

    // Bit-level primitives
    void alignBit();
    uint32_t readUB(uint8_t numBits);
    int32_t readSB(uint8_t numBits);

    // SWF specific composite types
    SwfRect readRect();
    std::string readString(); // Null-terminated string
    std::vector<uint8_t> readBytes(size_t count);

    const uint8_t* currentPtr() const;

private:
    const uint8_t* m_data;
    size_t m_size{0};
    size_t m_pos{0};

    uint8_t m_bitBuffer{0};
    uint8_t m_bitsRemaining{0};

    void checkBounds(size_t needed) const;
};

} // namespace btd4::swf
