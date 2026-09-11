#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace btd4::swf {

enum class Compression : uint8_t {
    None,  // 'FWS'
    Zlib,  // 'CWS'
    Lzma   // 'ZWS'
};

struct SwfRect {
    int32_t xMinTwips{0};
    int32_t xMaxTwips{0};
    int32_t yMinTwips{0};
    int32_t yMaxTwips{0};

    float widthPixels() const { return static_cast<float>(xMaxTwips - xMinTwips) / 20.0f; }
    float heightPixels() const { return static_cast<float>(yMaxTwips - yMinTwips) / 20.0f; }
};

struct SwfHeader {
    Compression compression{Compression::None};
    uint8_t version{0};
    uint32_t uncompressedLength{0};
    SwfRect frameSize;
    float frameRate{0.0f};
    uint16_t frameCount{0};
};

enum class TagCode : uint16_t {
    End                 = 0,
    ShowFrame           = 1,
    DefineShape         = 2,
    SetBackgroundColor  = 9,
    DefineSound         = 14,
    DefineBitsLossless  = 20,
    DefineBitsJPEG2     = 21,
    DefineShape2        = 22,
    DefineBitsJPEG3     = 35,
    DefineBitsLossless2 = 36,
    DefineSprite        = 39,
    FileAttributes      = 69,
    SymbolClass         = 76,
    DoABC               = 82,
    DefineShape4        = 83,
    DefineBitsJPEG4     = 90
};

enum class ImageFormat {
    JPEG,
    PNG,
    RGBA
};

struct SwfImage {
    uint16_t characterId{0};
    std::string className;
    uint32_t width{0};
    uint32_t height{0};
    ImageFormat format{ImageFormat::PNG};
    std::vector<uint8_t> data;
};

enum class SoundFormat : uint8_t {
    RawPCM,
    ADPCM,
    MP3,
    RawLittleEndianPCM,
    Nellymoser,
    Speex
};

struct SwfSound {
    uint16_t characterId{0};
    std::string className;
    SoundFormat format{SoundFormat::MP3};
    uint32_t sampleRate{44100};
    bool is16Bit{true};
    bool isStereo{true};
    uint32_t sampleCount{0};
    std::vector<uint8_t> data;
};

struct SwfSymbol {
    uint16_t characterId{0};
    std::string className;
};

struct SwfMetadata {
    uint8_t version{0};
    float width{0.0f};
    float height{0.0f};
    float frameRate{0.0f};
    uint16_t frameCount{0};
    bool isActionScript3{false};
    uint32_t imageCount{0};
    uint32_t soundCount{0};
    uint32_t symbolCount{0};
    std::vector<std::string> symbolNames;
};

} // namespace btd4::swf
