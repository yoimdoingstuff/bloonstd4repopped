#include "TestRunner.hpp"
#include "../tools/swf/SwfParser.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace {
void appendTag(std::vector<uint8_t>& swf, uint16_t code, const std::vector<uint8_t>& payload) {
    const uint16_t header = static_cast<uint16_t>((code << 6) | payload.size());
    swf.push_back(static_cast<uint8_t>(header & 0xFF));
    swf.push_back(static_cast<uint8_t>((header >> 8) & 0xFF));
    swf.insert(swf.end(), payload.begin(), payload.end());
}

std::vector<uint8_t> makeHeader() {
    return {'F','W','S',10,0,0,0,0,0x28,0x05,0x00,0x50,0x00,0x3C,0x01,0x00};
}

void patchFileLength(std::vector<uint8_t>& swf) {
    const uint32_t length = static_cast<uint32_t>(swf.size());
    swf[4] = static_cast<uint8_t>(length & 0xFF);
    swf[5] = static_cast<uint8_t>((length >> 8) & 0xFF);
    swf[6] = static_cast<uint8_t>((length >> 16) & 0xFF);
    swf[7] = static_cast<uint8_t>((length >> 24) & 0xFF);
}
}

TEST_CASE(SwfParserResolvesBitmapThroughShapeAndSprite) {
    std::vector<uint8_t> swf = makeHeader();

    // Bitmap character 20.
    appendTag(swf, 21, {20, 0, 0xFF, 0xD8});

    // Shape character 30 with one bitmap fill referencing character 20.
    const std::vector<uint8_t> shape = {
        30, 0,
        0x28, 0x05, 0x00, 0x50,
        1,
        0x40,
        20, 0,
        0x00, 0x00, 0x00
    };
    appendTag(swf, 2, shape);

    // Sprite character 40 placing shape 30 at depth 1.
    const std::vector<uint8_t> placeObject2 = {0x02, 1, 0, 30, 0};
    std::vector<uint8_t> sprite = {40, 0, 1, 0};
    const uint16_t placeHeader = static_cast<uint16_t>((26u << 6) | placeObject2.size());
    sprite.push_back(static_cast<uint8_t>(placeHeader & 0xFF));
    sprite.push_back(static_cast<uint8_t>(placeHeader >> 8));
    sprite.insert(sprite.end(), placeObject2.begin(), placeObject2.end());
    sprite.push_back(0x40); sprite.push_back(0x00); // ShowFrame
    sprite.push_back(0x00); sprite.push_back(0x00); // End
    appendTag(swf, 39, sprite);

    // Export sprite character 40 under a gameplay-like class name.
    appendTag(swf, 76, {1, 0, 40, 0, 'D','a','r','t','M','o','n','k','e','y',0});
    appendTag(swf, 0, {});
    patchFileLength(swf);

    btd4::swf::SwfParser parser;
    std::string error;
    TEST_ASSERT(parser.parse(swf.data(), swf.size(), error));
    TEST_ASSERT_EQ(parser.images().size(), static_cast<size_t>(1));
    TEST_ASSERT_EQ(parser.images().front().characterId, static_cast<uint16_t>(20));
    TEST_ASSERT_EQ(parser.images().front().className, "DartMonkey");
}
