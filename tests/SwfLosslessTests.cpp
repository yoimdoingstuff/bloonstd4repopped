#include "TestRunner.hpp"
#include "../tools/swf/SwfParser.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace {
uint32_t adler32(const std::vector<uint8_t>& data) {
    uint32_t a = 1;
    uint32_t b = 0;
    for (uint8_t byte : data) { a = (a + byte) % 65521u; b = (b + a) % 65521u; }
    return (b << 16) | a;
}

std::vector<uint8_t> zlibStored(const std::vector<uint8_t>& payload) {
    TEST_ASSERT(payload.size() <= 65535u);
    std::vector<uint8_t> out = {0x78, 0x01, 0x01,
        static_cast<uint8_t>(payload.size() & 0xFF), static_cast<uint8_t>((payload.size() >> 8) & 0xFF),
        static_cast<uint8_t>(~payload.size() & 0xFF), static_cast<uint8_t>((~payload.size() >> 8) & 0xFF)};
    out.insert(out.end(), payload.begin(), payload.end());
    const uint32_t checksum = adler32(payload);
    out.push_back(static_cast<uint8_t>((checksum >> 24) & 0xFF)); out.push_back(static_cast<uint8_t>((checksum >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((checksum >> 8) & 0xFF)); out.push_back(static_cast<uint8_t>(checksum & 0xFF));
    return out;
}

void appendTag(std::vector<uint8_t>& swf, uint16_t code, const std::vector<uint8_t>& payload) {
    const uint16_t header = static_cast<uint16_t>((code << 6) | payload.size());
    swf.push_back(static_cast<uint8_t>(header & 0xFF)); swf.push_back(static_cast<uint8_t>((header >> 8) & 0xFF)); swf.insert(swf.end(), payload.begin(), payload.end());
}

void patchFileLength(std::vector<uint8_t>& swf) {
    const uint32_t length = static_cast<uint32_t>(swf.size());
    swf[4] = static_cast<uint8_t>(length & 0xFF); swf[5] = static_cast<uint8_t>((length >> 8) & 0xFF);
    swf[6] = static_cast<uint8_t>((length >> 16) & 0xFF); swf[7] = static_cast<uint8_t>((length >> 24) & 0xFF);
}
}

TEST_CASE(SwfParserDecodesLossless32BitToRgba) {
    std::vector<uint8_t> swf = {'F','W','S',10,0,0,0,0,0x28,0x05,0x00,0x50,0x00,0x3C,0x01,0x00};
    const std::vector<uint8_t> rawPixels = {128, 255, 0, 0};
    std::vector<uint8_t> payload = {20, 0, 5, 1, 0, 1, 0};
    const std::vector<uint8_t> compressed = zlibStored(rawPixels); payload.insert(payload.end(), compressed.begin(), compressed.end());
    appendTag(swf, 36, payload); appendTag(swf, 0, {}); patchFileLength(swf);
    btd4::swf::SwfParser parser; std::string error;
    TEST_ASSERT(parser.parse(swf.data(), swf.size(), error)); TEST_ASSERT_EQ(parser.images().size(), static_cast<size_t>(1));
    TEST_ASSERT_EQ(parser.images()[0].width, static_cast<uint32_t>(1)); TEST_ASSERT_EQ(parser.images()[0].height, static_cast<uint32_t>(1));
    TEST_ASSERT_EQ(parser.images()[0].data.size(), static_cast<size_t>(4)); TEST_ASSERT_EQ(parser.images()[0].data[0], static_cast<uint8_t>(255));
    TEST_ASSERT_EQ(parser.images()[0].data[1], static_cast<uint8_t>(0)); TEST_ASSERT_EQ(parser.images()[0].data[2], static_cast<uint8_t>(0)); TEST_ASSERT_EQ(parser.images()[0].data[3], static_cast<uint8_t>(128));
}

TEST_CASE(SwfParserDecodesLossless16BitRgb15) {
    std::vector<uint8_t> swf = {'F','W','S',10,0,0,0,0,0x28,0x05,0x00,0x50,0x00,0x3C,0x01,0x00};
    const uint16_t rgb15 = static_cast<uint16_t>(31u << 5);
    const std::vector<uint8_t> rawPixels = {static_cast<uint8_t>(rgb15 >> 8), static_cast<uint8_t>(rgb15 & 0xFF), 0, 0};
    std::vector<uint8_t> payload = {21, 0, 4, 1, 0, 1, 0};
    const std::vector<uint8_t> compressed = zlibStored(rawPixels); payload.insert(payload.end(), compressed.begin(), compressed.end());
    appendTag(swf, 20, payload); appendTag(swf, 0, {}); patchFileLength(swf);
    btd4::swf::SwfParser parser; std::string error;
    TEST_ASSERT(parser.parse(swf.data(), swf.size(), error)); TEST_ASSERT_EQ(parser.images().size(), static_cast<size_t>(1));
    TEST_ASSERT(parser.images()[0].data[1] > 240); TEST_ASSERT_EQ(parser.images()[0].data[0], static_cast<uint8_t>(0));
    TEST_ASSERT_EQ(parser.images()[0].data[2], static_cast<uint8_t>(0)); TEST_ASSERT_EQ(parser.images()[0].data[3], static_cast<uint8_t>(255));
}
