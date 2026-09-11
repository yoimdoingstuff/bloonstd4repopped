#include "Inflate.hpp"
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace btd4::swf {

namespace {

// Bit-stream reader for DEFLATE
class BitReader {
public:
    BitReader(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}

    uint32_t readBits(size_t count) {
        while (m_bitCount < count) {
            if (m_bytePos >= m_size) {
                return 0; // EOF
            }
            m_bitBuffer |= (static_cast<uint64_t>(m_data[m_bytePos++]) << m_bitCount);
            m_bitCount += 8;
        }
        uint32_t val = static_cast<uint32_t>(m_bitBuffer & ((1ULL << count) - 1ULL));
        m_bitBuffer >>= count;
        m_bitCount -= count;
        return val;
    }

    void alignToByte() {
        m_bitBuffer = 0;
        m_bitCount = 0;
    }

    size_t bytesLeft() const {
        return (m_bytePos < m_size) ? (m_size - m_bytePos) : 0;
    }

    const uint8_t* currentBytePtr() const {
        return m_data + m_bytePos;
    }

    void advanceBytes(size_t n) {
        m_bytePos = std::min(m_size, m_bytePos + n);
    }

private:
    const uint8_t* m_data;
    size_t m_size{0};
    size_t m_bytePos{0};
    uint64_t m_bitBuffer{0};
    size_t m_bitCount{0};
};

// Huffman decoding tree/table
struct HuffmanTable {
    uint16_t counts[16]{};
    uint16_t symbols[288]{};

    bool build(const uint8_t* lengths, size_t numSymbols) {
        std::memset(counts, 0, sizeof(counts));
        for (size_t i = 0; i < numSymbols; ++i) {
            if (lengths[i] > 15) return false;
            counts[lengths[i]]++;
        }
        counts[0] = 0;

        uint16_t offsets[16]{};
        uint16_t start = 0;
        for (int len = 1; len <= 15; ++len) {
            start = (start + counts[len - 1]) << 1;
            offsets[len] = start;
        }

        for (size_t i = 0; i < numSymbols; ++i) {
            uint8_t len = lengths[i];
            if (len != 0) {
                symbols[offsets[len]++] = static_cast<uint16_t>(i);
            }
        }
        return true;
    }

    int decode(BitReader& reader) const {
        uint16_t code = 0;
        uint16_t first = 0;
        uint16_t index = 0;

        for (int len = 1; len <= 15; ++len) {
            code = (code << 1) | reader.readBits(1);
            int count = counts[len];
            if (code < first + count) {
                return symbols[index + (code - first)];
            }
            index += count;
            first = (first + count) << 1;
        }
        return -1; // invalid
    }
};

static const uint16_t s_lengthBases[] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
static const uint8_t s_lengthExtraBits[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

static const uint16_t s_distBases[] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
static const uint8_t s_distExtraBits[] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

static const uint8_t s_clOrder[] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

} // namespace

bool Inflate::decompressDeflate(const uint8_t* inData, size_t inSize, std::vector<uint8_t>& outData, size_t expectedOutputSize) {
    if (!inData || inSize == 0) return false;

    BitReader reader(inData, inSize);
    if (expectedOutputSize > 0) {
        outData.reserve(expectedOutputSize);
    }

    bool isLastBlock = false;

    while (!isLastBlock) {
        isLastBlock = (reader.readBits(1) != 0);
        uint32_t btype = reader.readBits(2);

        if (btype == 0) {
            // Uncompressed block
            reader.alignToByte();
            if (reader.bytesLeft() < 4) return false;
            const uint8_t* ptr = reader.currentBytePtr();
            uint16_t len = ptr[0] | (static_cast<uint16_t>(ptr[1]) << 8);
            uint16_t nlen = ptr[2] | (static_cast<uint16_t>(ptr[3]) << 8);
            reader.advanceBytes(4);

            if ((len ^ 0xFFFF) != nlen) return false;
            if (reader.bytesLeft() < len) return false;

            ptr = reader.currentBytePtr();
            outData.insert(outData.end(), ptr, ptr + len);
            reader.advanceBytes(len);
        } else if (btype == 1 || btype == 2) {
            HuffmanTable litLenTable;
            HuffmanTable distTable;

            if (btype == 1) {
                // Fixed Huffman codes
                uint8_t litLengths[288];
                for (int i = 0; i <= 143; ++i) litLengths[i] = 8;
                for (int i = 144; i <= 255; ++i) litLengths[i] = 9;
                for (int i = 256; i <= 279; ++i) litLengths[i] = 7;
                for (int i = 280; i <= 287; ++i) litLengths[i] = 8;
                if (!litLenTable.build(litLengths, 288)) return false;

                uint8_t distLengths[32];
                for (int i = 0; i < 32; ++i) distLengths[i] = 5;
                if (!distTable.build(distLengths, 32)) return false;
            } else {
                // Dynamic Huffman codes
                uint32_t hlit = reader.readBits(5) + 257;
                uint32_t hdist = reader.readBits(5) + 1;
                uint32_t hclen = reader.readBits(4) + 4;

                uint8_t codeLengths[19]{};
                for (uint32_t i = 0; i < hclen; ++i) {
                    codeLengths[s_clOrder[i]] = static_cast<uint8_t>(reader.readBits(3));
                }

                HuffmanTable clTable;
                if (!clTable.build(codeLengths, 19)) return false;

                uint8_t allLengths[320]{};
                size_t numTotal = hlit + hdist;
                size_t idx = 0;

                while (idx < numTotal) {
                    int sym = clTable.decode(reader);
                    if (sym < 0) return false;

                    if (sym < 16) {
                        allLengths[idx++] = static_cast<uint8_t>(sym);
                    } else if (sym == 16) {
                        if (idx == 0) return false;
                        uint8_t prev = allLengths[idx - 1];
                        uint32_t repeat = reader.readBits(2) + 3;
                        while (repeat-- > 0 && idx < numTotal) allLengths[idx++] = prev;
                    } else if (sym == 17) {
                        uint32_t repeat = reader.readBits(3) + 3;
                        while (repeat-- > 0 && idx < numTotal) allLengths[idx++] = 0;
                    } else if (sym == 18) {
                        uint32_t repeat = reader.readBits(7) + 11;
                        while (repeat-- > 0 && idx < numTotal) allLengths[idx++] = 0;
                    }
                }

                if (!litLenTable.build(allLengths, hlit)) return false;
                if (!distTable.build(allLengths + hlit, hdist)) return false;
            }

            // Decompress symbols
            while (true) {
                int sym = litLenTable.decode(reader);
                if (sym < 0 || sym > 285) return false;

                if (sym < 256) {
                    outData.push_back(static_cast<uint8_t>(sym));
                } else if (sym == 256) {
                    break; // End of block
                } else {
                    int lenCode = sym - 257;
                    uint32_t length = s_lengthBases[lenCode];
                    uint8_t extraBits = s_lengthExtraBits[lenCode];
                    if (extraBits > 0) length += reader.readBits(extraBits);

                    int distCode = distTable.decode(reader);
                    if (distCode < 0 || distCode >= 30) return false;
                    uint32_t dist = s_distBases[distCode];
                    uint8_t distExtra = s_distExtraBits[distCode];
                    if (distExtra > 0) dist += reader.readBits(distExtra);

                    if (dist > outData.size()) return false; // distance too far back

                    size_t startPos = outData.size() - dist;
                    for (uint32_t k = 0; k < length; ++k) {
                        outData.push_back(outData[startPos + k]);
                    }
                }
            }
        } else {
            // Reserved / invalid
            return false;
        }
    }

    return true;
}

bool Inflate::decompressZlib(const uint8_t* inData, size_t inSize, std::vector<uint8_t>& outData, size_t expectedOutputSize) {
    if (!inData || inSize < 6) return false; // 2 byte header + 4 byte Adler32

    // Check zlib header: CMF and FLG
    uint8_t cmf = inData[0];
    uint8_t flg = inData[1];

    if ((cmf & 0x0F) != 8) return false; // compression method must be 8 (deflate)
    if (((cmf * 256 + flg) % 31) != 0) return false; // header checksum check
    if (flg & 0x20) return false; // preset dictionary not supported

    // Decompress payload (excluding 2-byte header and 4-byte Adler32)
    size_t payloadSize = inSize - 6;
    return decompressDeflate(inData + 2, payloadSize, outData, expectedOutputSize);
}

} // namespace btd4::swf
