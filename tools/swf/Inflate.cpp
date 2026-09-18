#include "Inflate.hpp"
#include <algorithm>
#include <array>
#include <cstdint>

namespace btd4::swf {

namespace {

constexpr size_t kMaxInflatedOutput = 256u * 1024u * 1024u;

class BitReader {
public:
    BitReader(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}

    bool readBits(size_t count, uint32_t& value) {
        if (count > 32) return false;
        value = 0;
        while (m_bitCount < count) {
            if (m_bytePos >= m_size) return false;
            m_bitBuffer |= static_cast<uint64_t>(m_data[m_bytePos++]) << m_bitCount;
            m_bitCount += 8;
        }
        if (count == 32) value = static_cast<uint32_t>(m_bitBuffer);
        else if (count != 0) value = static_cast<uint32_t>(m_bitBuffer & ((1ULL << count) - 1ULL));
        m_bitBuffer >>= count;
        m_bitCount -= count;
        return true;
    }

    bool readBit(uint32_t& value) { return readBits(1, value); }

    void alignToByte() { m_bitBuffer = 0; m_bitCount = 0; }
    size_t bytesLeft() const { return m_bytePos < m_size ? m_size - m_bytePos : 0; }
    const uint8_t* currentBytePtr() const { return m_bytePos <= m_size ? m_data + m_bytePos : nullptr; }

    void advanceBytes(size_t count) {
        m_bytePos = count > bytesLeft() ? m_size : m_bytePos + count;
    }

private:
    const uint8_t* m_data{nullptr};
    size_t m_size{0};
    size_t m_bytePos{0};
    uint64_t m_bitBuffer{0};
    size_t m_bitCount{0};
};

uint32_t reverseBits(uint32_t value, uint8_t count) {
    uint32_t result = 0;
    for (uint8_t i = 0; i < count; ++i) result = (result << 1) | ((value >> i) & 1u);
    return result;
}

struct HuffmanTable {
    std::array<uint16_t, 16> counts{};
    std::array<uint16_t, 288> symbols{};
    std::array<uint16_t, 288> reversedCodes{};

    bool build(const uint8_t* lengths, size_t numSymbols) {
        if (!lengths || numSymbols == 0 || numSymbols > symbols.size()) return false;

        counts.fill(0);
        for (size_t i = 0; i < numSymbols; ++i) {
            if (lengths[i] > 15) return false;
            ++counts[lengths[i]];
        }
        counts[0] = 0;

        uint32_t code = 0;
        std::array<uint32_t, 16> nextCode{};
        for (uint8_t len = 1; len <= 15; ++len) {
            code = (code + counts[len - 1]) << 1;
            if (code + counts[len] > (1u << len)) return false;
            nextCode[len] = code;
        }

        std::array<uint16_t, 16> offsets{};
        uint16_t offset = 0;
        for (uint8_t len = 1; len <= 15; ++len) {
            offsets[len] = offset;
            offset = static_cast<uint16_t>(offset + counts[len]);
        }

        auto writeOffsets = offsets;
        for (size_t symbol = 0; symbol < numSymbols; ++symbol) {
            const uint8_t len = lengths[symbol];
            if (len == 0) continue;
            const uint32_t canonical = nextCode[len]++;
            const uint16_t index = writeOffsets[len]++;
            symbols[index] = static_cast<uint16_t>(symbol);
            reversedCodes[index] = static_cast<uint16_t>(reverseBits(canonical, len));
        }
        return true;
    }

    int decode(BitReader& reader) const {
        uint32_t code = 0;
        size_t index = 0;
        for (uint8_t len = 1; len <= 15; ++len) {
            uint32_t bit = 0;
            if (!reader.readBit(bit)) return -1;
            code |= bit << (len - 1);
            const size_t count = counts[len];
            for (size_t i = 0; i < count; ++i) {
                if (reversedCodes[index + i] == code) return symbols[index + i];
            }
            index += count;
        }
        return -1;
    }
};

constexpr uint16_t s_lengthBases[] = {
    3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258
};
constexpr uint8_t s_lengthExtraBits[] = {
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0
};
constexpr uint16_t s_distBases[] = {
    1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577
};
constexpr uint8_t s_distExtraBits[] = {
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13
};
constexpr uint8_t s_clOrder[] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};

uint32_t adler32(const uint8_t* data, size_t size) {
    constexpr uint32_t mod = 65521u;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < size; ++i) {
        a += data[i];
        if (a >= mod) a -= mod;
        b += a;
        if (b >= mod) b -= mod;
    }
    return (b << 16) | a;
}

} // namespace

bool Inflate::decompressDeflate(const uint8_t* inData, size_t inSize,
                                std::vector<uint8_t>& outData,
                                size_t expectedOutputSize) {
    if (!inData || inSize == 0 || expectedOutputSize > kMaxInflatedOutput) return false;
    outData.clear();
    if (expectedOutputSize > 0) outData.reserve(expectedOutputSize);

    BitReader reader(inData, inSize);
    bool lastBlock = false;
    while (!lastBlock) {
        uint32_t last = 0, blockType = 0;
        if (!reader.readBit(last) || !reader.readBits(2, blockType)) return false;
        lastBlock = last != 0;

        if (blockType == 0) {
            reader.alignToByte();
            if (reader.bytesLeft() < 4) return false;
            const uint8_t* ptr = reader.currentBytePtr();
            const uint16_t length = static_cast<uint16_t>(ptr[0]) | (static_cast<uint16_t>(ptr[1]) << 8);
            const uint16_t inverse = static_cast<uint16_t>(ptr[2]) | (static_cast<uint16_t>(ptr[3]) << 8);
            if ((length ^ 0xFFFFu) != inverse) return false;
            reader.advanceBytes(4);
            if (reader.bytesLeft() < length || outData.size() > kMaxInflatedOutput - length) return false;
            ptr = reader.currentBytePtr();
            outData.insert(outData.end(), ptr, ptr + length);
            reader.advanceBytes(length);
            continue;
        }

        if (blockType != 1 && blockType != 2) return false;

        HuffmanTable litLenTable, distTable;
        if (blockType == 1) {
            std::array<uint8_t, 288> litLengths{};
            for (int i = 0; i <= 143; ++i) litLengths[i] = 8;
            for (int i = 144; i <= 255; ++i) litLengths[i] = 9;
            for (int i = 256; i <= 279; ++i) litLengths[i] = 7;
            for (int i = 280; i <= 287; ++i) litLengths[i] = 8;
            std::array<uint8_t, 32> distLengths{};
            distLengths.fill(5);
            if (!litLenTable.build(litLengths.data(), litLengths.size()) ||
                !distTable.build(distLengths.data(), distLengths.size())) return false;
        } else {
            uint32_t hlit = 0, hdist = 0, hclen = 0;
            if (!reader.readBits(5, hlit) || !reader.readBits(5, hdist) || !reader.readBits(4, hclen)) return false;
            hlit += 257; hdist += 1; hclen += 4;
            if (hlit > 288 || hdist > 32 || hclen > 19) return false;

            std::array<uint8_t, 19> codeLengths{};
            for (uint32_t i = 0; i < hclen; ++i) {
                uint32_t v = 0;
                if (!reader.readBits(3, v)) return false;
                codeLengths[s_clOrder[i]] = static_cast<uint8_t>(v);
            }

            HuffmanTable codeLengthTable;
            if (!codeLengthTable.build(codeLengths.data(), codeLengths.size())) return false;

            std::array<uint8_t, 320> allLengths{};
            const size_t total = static_cast<size_t>(hlit + hdist);
            size_t index = 0;
            while (index < total) {
                const int symbol = codeLengthTable.decode(reader);
                if (symbol < 0) return false;
                if (symbol < 16) {
                    allLengths[index++] = static_cast<uint8_t>(symbol);
                } else if (symbol == 16) {
                    if (index == 0) return false;
                    uint32_t repeat = 0;
                    if (!reader.readBits(2, repeat)) return false;
                    repeat += 3;
                    if (repeat > total - index) return false;
                    std::fill_n(allLengths.begin() + static_cast<std::ptrdiff_t>(index), repeat, allLengths[index - 1]);
                    index += repeat;
                } else if (symbol == 17 || symbol == 18) {
                    uint32_t repeat = 0;
                    const uint8_t extra = symbol == 17 ? 3 : 7;
                    if (!reader.readBits(extra, repeat)) return false;
                    repeat += symbol == 17 ? 3 : 11;
                    if (repeat > total - index) return false;
                    index += repeat;
                } else {
                    return false;
                }
            }

            if (!litLenTable.build(allLengths.data(), hlit) ||
                !distTable.build(allLengths.data() + hlit, hdist)) return false;
        }

        while (true) {
            const int symbol = litLenTable.decode(reader);
            if (symbol < 0 || symbol > 285) return false;
            if (symbol < 256) {
                if (outData.size() >= kMaxInflatedOutput) return false;
                outData.push_back(static_cast<uint8_t>(symbol));
                continue;
            }
            if (symbol == 256) break;

            const int lengthCode = symbol - 257;
            if (lengthCode < 0 || lengthCode >= 29) return false;
            uint32_t length = s_lengthBases[lengthCode];
            if (s_lengthExtraBits[lengthCode] != 0) {
                uint32_t extra = 0;
                if (!reader.readBits(s_lengthExtraBits[lengthCode], extra)) return false;
                length += extra;
            }

            const int distanceCode = distTable.decode(reader);
            if (distanceCode < 0 || distanceCode >= 30) return false;
            uint32_t distance = s_distBases[distanceCode];
            if (s_distExtraBits[distanceCode] != 0) {
                uint32_t extra = 0;
                if (!reader.readBits(s_distExtraBits[distanceCode], extra)) return false;
                distance += extra;
            }
            if (distance == 0 || distance > outData.size() ||
                length > kMaxInflatedOutput - outData.size()) return false;

            const size_t start = outData.size() - distance;
            for (uint32_t i = 0; i < length; ++i) {
                outData.push_back(outData[start + i]);
            }
        }
    }

    return expectedOutputSize == 0 || outData.size() == expectedOutputSize;
}

bool Inflate::decompressZlib(const uint8_t* inData, size_t inSize,
                             std::vector<uint8_t>& outData,
                             size_t expectedOutputSize) {
    if (!inData || inSize < 6 || expectedOutputSize > kMaxInflatedOutput) return false;

    const uint8_t cmf = inData[0], flg = inData[1];
    if ((cmf & 0x0F) != 8) return false;
    if ((((static_cast<unsigned>(cmf) << 8) | flg) % 31) != 0) return false;
    if (flg & 0x20) return false;

    std::vector<uint8_t> decompressed;
    if (!decompressDeflate(inData + 2, inSize - 6, decompressed, expectedOutputSize)) return false;

    const uint32_t expectedAdler =
        (static_cast<uint32_t>(inData[inSize - 4]) << 24) |
        (static_cast<uint32_t>(inData[inSize - 3]) << 16) |
        (static_cast<uint32_t>(inData[inSize - 2]) << 8) |
        static_cast<uint32_t>(inData[inSize - 1]);
    if (adler32(decompressed.data(), decompressed.size()) != expectedAdler) return false;

    outData = std::move(decompressed);
    return true;
}

} // namespace btd4::swf
