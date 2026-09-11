#include "SwfReader.hpp"
#include <cstring>
#include <stdexcept>

namespace btd4::swf {

SwfReader::SwfReader(const uint8_t* data, size_t size)
    : m_data(data), m_size(size), m_pos(0) {
}

SwfReader::SwfReader(const std::vector<uint8_t>& buffer)
    : m_data(buffer.data()), m_size(buffer.size()), m_pos(0) {
}

void SwfReader::checkBounds(size_t needed) const {
    if (m_pos + needed > m_size) {
        throw std::out_of_range("SwfReader: Unexpected end of buffer");
    }
}

void SwfReader::seek(size_t pos) {
    if (pos > m_size) {
        throw std::out_of_range("SwfReader: Seek out of range");
    }
    m_pos = pos;
    m_bitBuffer = 0;
    m_bitsRemaining = 0;
}

void SwfReader::skip(size_t bytes) {
    checkBounds(bytes);
    m_pos += bytes;
    m_bitBuffer = 0;
    m_bitsRemaining = 0;
}

const uint8_t* SwfReader::currentPtr() const {
    return (m_pos < m_size) ? (m_data + m_pos) : nullptr;
}

uint8_t SwfReader::readUI8() {
    checkBounds(1);
    alignBit();
    return m_data[m_pos++];
}

uint16_t SwfReader::readUI16() {
    checkBounds(2);
    alignBit();
    uint16_t val = static_cast<uint16_t>(m_data[m_pos]) |
                   (static_cast<uint16_t>(m_data[m_pos + 1]) << 8);
    m_pos += 2;
    return val;
}

uint32_t SwfReader::readUI32() {
    checkBounds(4);
    alignBit();
    uint32_t val = static_cast<uint32_t>(m_data[m_pos]) |
                   (static_cast<uint32_t>(m_data[m_pos + 1]) << 8) |
                   (static_cast<uint32_t>(m_data[m_pos + 2]) << 16) |
                   (static_cast<uint32_t>(m_data[m_pos + 3]) << 24);
    m_pos += 4;
    return val;
}

int8_t SwfReader::readSI8() {
    return static_cast<int8_t>(readUI8());
}

int16_t SwfReader::readSI16() {
    return static_cast<int16_t>(readUI16());
}

int32_t SwfReader::readSI32() {
    return static_cast<int32_t>(readUI32());
}

float SwfReader::readFixed8_8() {
    uint16_t raw = readUI16();
    return static_cast<float>(raw) / 256.0f;
}

uint32_t SwfReader::readU30() {
    return readU32() & 0x3FFFFFFFU;
}

uint32_t SwfReader::readU32() {
    uint32_t result = 0;
    uint32_t shift = 0;
    while (shift < 35) {
        uint8_t b = readUI8();
        result |= static_cast<uint32_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0) break;
        shift += 7;
    }
    return result;
}

void SwfReader::alignBit() {
    m_bitBuffer = 0;
    m_bitsRemaining = 0;
}

uint32_t SwfReader::readUB(uint8_t numBits) {
    if (numBits == 0) return 0;
    if (numBits > 32) numBits = 32;

    uint32_t result = 0;
    while (numBits > 0) {
        if (m_bitsRemaining == 0) {
            checkBounds(1);
            m_bitBuffer = m_data[m_pos++];
            m_bitsRemaining = 8;
        }

        uint8_t bitsToTake = (numBits < m_bitsRemaining) ? numBits : m_bitsRemaining;
        uint8_t shift = m_bitsRemaining - bitsToTake;
        uint8_t mask = (1 << bitsToTake) - 1;
        uint8_t chunk = (m_bitBuffer >> shift) & mask;

        result = (result << bitsToTake) | chunk;
        numBits -= bitsToTake;
        m_bitsRemaining -= bitsToTake;
    }
    return result;
}

int32_t SwfReader::readSB(uint8_t numBits) {
    if (numBits == 0) return 0;
    uint32_t val = readUB(numBits);
    // Sign-extend if top bit is 1
    if (val & (1U << (numBits - 1))) {
        val |= ~((1U << numBits) - 1);
    }
    return static_cast<int32_t>(val);
}

SwfRect SwfReader::readRect() {
    alignBit();
    uint8_t nBits = static_cast<uint8_t>(readUB(5));
    SwfRect rect;
    rect.xMinTwips = readSB(nBits);
    rect.xMaxTwips = readSB(nBits);
    rect.yMinTwips = readSB(nBits);
    rect.yMaxTwips = readSB(nBits);
    alignBit();
    return rect;
}

std::string SwfReader::readString() {
    alignBit();
    std::string str;
    while (true) {
        checkBounds(1);
        char c = static_cast<char>(m_data[m_pos++]);
        if (c == '\0') break;
        str.push_back(c);
    }
    return str;
}

std::vector<uint8_t> SwfReader::readBytes(size_t count) {
    checkBounds(count);
    alignBit();
    std::vector<uint8_t> result(m_data + m_pos, m_data + m_pos + count);
    m_pos += count;
    return result;
}

} // namespace btd4::swf
