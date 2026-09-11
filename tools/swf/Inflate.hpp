#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace btd4::swf {

class Inflate {
public:
    // Decompresses raw DEFLATE or zlib-wrapped stream into outBuffer
    static bool decompressZlib(const uint8_t* inData, size_t inSize, std::vector<uint8_t>& outData, size_t expectedOutputSize = 0);
    static bool decompressDeflate(const uint8_t* inData, size_t inSize, std::vector<uint8_t>& outData, size_t expectedOutputSize = 0);
};

} // namespace btd4::swf
