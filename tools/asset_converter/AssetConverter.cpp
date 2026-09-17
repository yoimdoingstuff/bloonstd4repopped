#include "AssetConverter.hpp"
#include <fstream>
#include <cctype>
#include <algorithm>

namespace btd4::tools {

namespace {

void writeWavHeader(std::ofstream& out, uint32_t sampleRate, uint16_t numChannels, uint16_t bitsPerSample, uint32_t dataSize) {
    uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    uint32_t chunkSize = 36 + dataSize;
    out.write("RIFF", 4); out.write(reinterpret_cast<const char*>(&chunkSize), 4); out.write("WAVE", 4);
    out.write("fmt ", 4); uint32_t subchunk1Size = 16; uint16_t audioFormat = 1;
    out.write(reinterpret_cast<const char*>(&subchunk1Size), 4); out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    out.write(reinterpret_cast<const char*>(&numChannels), 2); out.write(reinterpret_cast<const char*>(&sampleRate), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4); out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2); out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), 4);
}

void writeBmpHeader(std::ofstream& out, uint32_t width, uint32_t height) {
    uint32_t rowSize = width * 4; uint32_t imageSize = rowSize * height; uint32_t fileSize = 54 + imageSize; uint32_t dataOffset = 54;
    out.put('B'); out.put('M'); out.write(reinterpret_cast<const char*>(&fileSize), 4); uint32_t reserved = 0;
    out.write(reinterpret_cast<const char*>(&reserved), 4); out.write(reinterpret_cast<const char*>(&dataOffset), 4);
    uint32_t headerSize = 40; int32_t w = static_cast<int32_t>(width); int32_t h = -static_cast<int32_t>(height);
    uint16_t planes = 1; uint16_t bpp = 32; uint32_t compression = 0; uint32_t xPels = 2835; uint32_t yPels = 2835; uint32_t clrUsed = 0; uint32_t clrImportant = 0;
    out.write(reinterpret_cast<const char*>(&headerSize), 4); out.write(reinterpret_cast<const char*>(&w), 4); out.write(reinterpret_cast<const char*>(&h), 4);
    out.write(reinterpret_cast<const char*>(&planes), 2); out.write(reinterpret_cast<const char*>(&bpp), 2); out.write(reinterpret_cast<const char*>(&compression), 4);
    out.write(reinterpret_cast<const char*>(&imageSize), 4); out.write(reinterpret_cast<const char*>(&xPels), 4); out.write(reinterpret_cast<const char*>(&yPels), 4);
    out.write(reinterpret_cast<const char*>(&clrUsed), 4); out.write(reinterpret_cast<const char*>(&clrImportant), 4);
}

} // namespace

bool AssetConverter::saveImage(const swf::SwfImage& image, const std::string& outputPath) {
    if (image.data.empty()) return false;
    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) return false;
    if (image.format == swf::ImageFormat::JPEG) {
        out.write(reinterpret_cast<const char*>(image.data.data()), static_cast<std::streamsize>(image.data.size()));
        return static_cast<bool>(out);
    }

    const uint32_t w = image.width > 0 ? image.width : 1;
    const uint32_t h = image.height > 0 ? image.height : 1;
    const size_t expected = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
    if (image.format != swf::ImageFormat::RGBA || image.data.size() < expected) return false;

    writeBmpHeader(out, w, h);
    std::vector<uint8_t> bgra(expected);
    for (size_t i = 0; i < static_cast<size_t>(w) * static_cast<size_t>(h); ++i) {
        const uint8_t r = image.data[i * 4 + 0];
        const uint8_t g = image.data[i * 4 + 1];
        const uint8_t b = image.data[i * 4 + 2];
        const uint8_t a = image.data[i * 4 + 3];
        bgra[i * 4 + 0] = b; bgra[i * 4 + 1] = g; bgra[i * 4 + 2] = r; bgra[i * 4 + 3] = a;
    }
    out.write(reinterpret_cast<const char*>(bgra.data()), static_cast<std::streamsize>(bgra.size()));
    return static_cast<bool>(out);
}

bool AssetConverter::saveSound(const swf::SwfSound& sound, const std::string& outputPath) {
    if (sound.data.empty()) return false;
    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) return false;
    if (sound.format == swf::SoundFormat::MP3) {
        const size_t offset = sound.data.size() > 2 ? 2 : 0;
        out.write(reinterpret_cast<const char*>(sound.data.data() + offset), static_cast<std::streamsize>(sound.data.size() - offset));
        return static_cast<bool>(out);
    }
    const uint16_t numChannels = sound.isStereo ? 2 : 1;
    const uint16_t bitsPerSample = sound.is16Bit ? 16 : 8;
    writeWavHeader(out, sound.sampleRate, numChannels, bitsPerSample, static_cast<uint32_t>(sound.data.size()));
    out.write(reinterpret_cast<const char*>(sound.data.data()), static_cast<std::streamsize>(sound.data.size()));
    return static_cast<bool>(out);
}

std::string AssetConverter::normalizeIdentifier(const std::string& rawName, const std::string& fallbackPrefix, uint16_t id) {
    if (rawName.empty()) return fallbackPrefix + "_" + std::to_string(id);
    std::string result; result.reserve(rawName.size() + 4);
    for (size_t i = 0; i < rawName.size(); ++i) {
        const char c = rawName[i];
        if (std::isupper(static_cast<unsigned char>(c))) {
            if (i > 0 && rawName[i - 1] != '_' && !std::isupper(static_cast<unsigned char>(rawName[i - 1]))) result.push_back('_');
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        } else if (std::isalnum(static_cast<unsigned char>(c))) result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        else if (c == '_' || c == '-') result.push_back('_');
    }
    return result.empty() ? (fallbackPrefix + "_" + std::to_string(id)) : result;
}

} // namespace btd4::tools
