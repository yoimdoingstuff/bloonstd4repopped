#include "SwfParser.hpp"
#include "SwfReader.hpp"
#include "Inflate.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_set>

namespace btd4::swf {
namespace {

void skipMatrix(SwfReader& reader) {
    const uint8_t hasScaleBits = static_cast<uint8_t>(reader.readUB(5));
    if (hasScaleBits > 0) {
        reader.readSB(hasScaleBits);
        reader.readSB(hasScaleBits);
    }
    const uint8_t hasRotateBits = static_cast<uint8_t>(reader.readUB(5));
    if (hasRotateBits > 0) {
        reader.readSB(hasRotateBits);
        reader.readSB(hasRotateBits);
    }
    const uint8_t translateBits = static_cast<uint8_t>(reader.readUB(5));
    if (translateBits > 0) {
        reader.readSB(translateBits);
        reader.readSB(translateBits);
    }
    reader.alignBit();
}

void skipColor(SwfReader& reader, bool hasAlpha) { reader.skip(hasAlpha ? 4 : 3); }

void skipGradient(SwfReader& reader, bool hasAlpha) {
    const uint8_t gradientHeader = reader.readUI8();
    const uint8_t count = static_cast<uint8_t>(gradientHeader & 0x0F);
    for (uint8_t i = 0; i < count; ++i) {
        reader.readUI8();
        skipColor(reader, hasAlpha);
    }
}

void skipFillStyle(SwfReader& reader, uint16_t tagCode, std::vector<uint16_t>& bitmapRefs) {
    const uint8_t type = reader.readUI8();
    const bool hasAlpha = tagCode >= static_cast<uint16_t>(TagCode::DefineShape3);
    if (type == 0x00) {
        skipColor(reader, hasAlpha);
        return;
    }
    if (type == 0x10 || type == 0x12 || type == 0x13) {
        skipMatrix(reader);
        skipGradient(reader, hasAlpha);
        if (type == 0x13) reader.skip(2);
        return;
    }
    if (type >= 0x40 && type <= 0x43) {
        const uint16_t bitmapId = reader.readUI16();
        bitmapRefs.push_back(bitmapId);
        skipMatrix(reader);
        return;
    }
    throw std::runtime_error("Unsupported SWF fill style");
}

std::vector<uint16_t> extractShapeBitmapRefs(const uint8_t* payload, size_t length, uint16_t tagCode) {
    SwfReader reader(payload, length);
    reader.readUI16();
    reader.readRect();
    if (tagCode == static_cast<uint16_t>(TagCode::DefineShape4)) {
        reader.readRect();
        reader.readUI8();
    }
    uint16_t fillStyleCount = reader.readUI8();
    if (fillStyleCount == 0xFF) fillStyleCount = reader.readUI16();
    std::vector<uint16_t> refs;
    for (uint16_t i = 0; i < fillStyleCount; ++i) skipFillStyle(reader, tagCode, refs);
    return refs;
}

void appendUnique(std::vector<uint16_t>& target, uint16_t value) {
    if (std::find(target.begin(), target.end(), value) == target.end()) target.push_back(value);
}

uint8_t scale5To8(uint8_t value) {
    return static_cast<uint8_t>((static_cast<uint16_t>(value) * 255u + 15u) / 31u);
}

bool checkedImageSize(uint32_t width, uint32_t height, size_t bytesPerPixel, size_t& outSize) {
    if (width == 0 || height == 0) return false;
    const size_t pixels = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (pixels > (static_cast<size_t>(-1) / bytesPerPixel)) return false;
    outSize = pixels * bytesPerPixel;
    return true;
}

bool decodeLosslessToRgba(const std::vector<uint8_t>& compressed, uint8_t format, uint32_t width,
                          uint32_t height, bool hasAlpha, std::vector<uint8_t>& rgba) {
    std::vector<uint8_t> src;
    if (!Inflate::decompressZlib(compressed.data(), compressed.size(), src)) return false;
    if (width == 0 || height == 0) return false;

    size_t rgbaSize = 0;
    if (!checkedImageSize(width, height, 4, rgbaSize)) return false;
    rgba.assign(rgbaSize, 0);

    if (format == 5) {
        const size_t stride = static_cast<size_t>(width) * 4;
        if (src.size() < stride * height) return false;
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const size_t s = static_cast<size_t>(y) * stride + static_cast<size_t>(x) * 4;
                const size_t d = (static_cast<size_t>(y) * width + x) * 4;
                rgba[d + 0] = src[s + 1];
                rgba[d + 1] = src[s + 2];
                rgba[d + 2] = src[s + 3];
                rgba[d + 3] = hasAlpha ? src[s + 0] : 255;
            }
        }
        return true;
    }

    if (format == 4) {
        const size_t stride = (static_cast<size_t>(width) * 2 + 3u) & ~static_cast<size_t>(3u);
        if (src.size() < stride * height) return false;
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const size_t s = static_cast<size_t>(y) * stride + static_cast<size_t>(x) * 2;
                const uint16_t packed = static_cast<uint16_t>(src[s]) | (static_cast<uint16_t>(src[s + 1]) << 8);
                const size_t d = (static_cast<size_t>(y) * width + x) * 4;
                rgba[d + 0] = scale5To8(static_cast<uint8_t>((packed >> 10) & 0x1F));
                rgba[d + 1] = scale5To8(static_cast<uint8_t>((packed >> 5) & 0x1F));
                rgba[d + 2] = scale5To8(static_cast<uint8_t>(packed & 0x1F));
                rgba[d + 3] = 255;
            }
        }
        return true;
    }

    if (format == 3) {
        if (src.empty()) return false;
        const size_t paletteEntrySize = hasAlpha ? 4 : 3;
        const size_t paletteCount = static_cast<size_t>(src[0]) + 1;
        const size_t paletteBytes = paletteCount * paletteEntrySize;
        if (paletteBytes > src.size() - 1) return false;
        const size_t pixelStart = 1 + paletteBytes;
        const size_t stride = (static_cast<size_t>(width) + 3u) & ~static_cast<size_t>(3u);
        if (pixelStart > src.size() || stride * height > src.size() - pixelStart) return false;
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const uint8_t index = src[pixelStart + static_cast<size_t>(y) * stride + x];
                if (index >= paletteCount) return false;
                const size_t p = 1 + static_cast<size_t>(index) * paletteEntrySize;
                const size_t d = (static_cast<size_t>(y) * width + x) * 4;
                rgba[d + 0] = src[p + 0];
                rgba[d + 1] = src[p + 1];
                rgba[d + 2] = src[p + 2];
                rgba[d + 3] = hasAlpha ? src[p + 3] : 255;
            }
        }
        return true;
    }

    return false;
}

} // namespace

SwfParser::SwfParser() = default;

std::string SwfParser::findSymbolName(uint16_t characterId) const {
    auto it = m_characterToSymbol.find(characterId);
    return (it != m_characterToSymbol.end()) ? it->second : "";
}

uint16_t SwfParser::findCharacterId(const std::string& className) const {
    auto it = m_symbolToCharacter.find(className);
    return (it != m_symbolToCharacter.end()) ? it->second : 0;
}

bool SwfParser::parseFile(const std::string& filePath, std::string& outError, ProgressCallback progress) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) { outError = "Could not open SWF file: " + filePath; return false; }
    std::streamsize fileSize = file.tellg(); file.seekg(0, std::ios::beg);
    if (fileSize < 8) { outError = "File is too small to be a valid SWF (< 8 bytes)"; return false; }
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) { outError = "Failed to read SWF file contents: " + filePath; return false; }
    return parse(buffer.data(), buffer.size(), outError, progress);
}

bool SwfParser::parse(const uint8_t* data, size_t size, std::string& outError, ProgressCallback progress) {
    if (!data || size < 8) { outError = "Invalid SWF buffer: too small (< 8 bytes)"; return false; }
    m_images.clear(); m_sounds.clear(); m_characterToSymbol.clear(); m_symbolToCharacter.clear(); m_characterBitmapRefs.clear(); m_characterChildren.clear(); m_warnings.clear(); m_metadata.symbolNames.clear();
    const char sig0 = static_cast<char>(data[0]); const char sig1 = static_cast<char>(data[1]); const char sig2 = static_cast<char>(data[2]);
    if (sig1 != 'W' || sig2 != 'S') { outError = "Invalid SWF signature (must be ?WS)"; return false; }
    if (sig0 == 'F') m_header.compression = Compression::None;
    else if (sig0 == 'C') m_header.compression = Compression::Zlib;
    else if (sig0 == 'Z') { m_header.compression = Compression::Lzma; outError = "LZMA-compressed SWF (ZWS) is currently not supported"; return false; }
    else { outError = "Unknown SWF compression signature: " + std::string(1, sig0); return false; }
    m_header.version = data[3];
    m_header.uncompressedLength = static_cast<uint32_t>(data[4]) | (static_cast<uint32_t>(data[5]) << 8) | (static_cast<uint32_t>(data[6]) << 16) | (static_cast<uint32_t>(data[7]) << 24);
    if (progress) progress(0.05f, "Decompressing SWF header...");
    std::vector<uint8_t> movieBuffer; const uint8_t* movieData = nullptr; size_t movieSize = 0;
    if (m_header.compression == Compression::Zlib) {
        const size_t compressedSize = size - 8; const size_t expectedSize = (m_header.uncompressedLength > 8) ? (m_header.uncompressedLength - 8) : 0;
        if (!Inflate::decompressZlib(data + 8, compressedSize, movieBuffer, expectedSize)) { outError = "Failed to decompress CWS zlib stream"; return false; }
        movieData = movieBuffer.data(); movieSize = movieBuffer.size();
    } else { movieData = data + 8; movieSize = size - 8; }
    if (movieSize < 4) { outError = "Uncompressed SWF payload is too small"; return false; }
    try {
        SwfReader headerReader(movieData, movieSize); m_header.frameSize = headerReader.readRect(); m_header.frameRate = headerReader.readFixed8_8(); m_header.frameCount = headerReader.readUI16();
        m_metadata.version = m_header.version; m_metadata.width = m_header.frameSize.widthPixels(); m_metadata.height = m_header.frameSize.heightPixels(); m_metadata.frameRate = m_header.frameRate; m_metadata.frameCount = m_header.frameCount;
        const size_t tagOffset = headerReader.position(); if (tagOffset < movieSize) return parseTags(movieData + tagOffset, movieSize - tagOffset, outError, progress);
    } catch (const std::exception& e) { outError = std::string("Error reading SWF movie header: ") + e.what(); return false; }
    return true;
}

bool SwfParser::parseTags(const uint8_t* tagData, size_t tagSize, std::string& outError, ProgressCallback progress) {
    SwfReader reader(tagData, tagSize); size_t tagIndex = 0;
    while (!reader.isEof()) {
        if (reader.remaining() < 2) break;
        const uint16_t tagWord = reader.readUI16(); const uint16_t tagCode = tagWord >> 6; uint32_t tagLength = tagWord & 0x3F;
        if (tagLength == 0x3F) { if (reader.remaining() < 4) { outError = "Truncated extended tag header at tag " + std::to_string(tagIndex); return false; } tagLength = reader.readUI32(); }
        if (reader.remaining() < tagLength) { m_warnings.push_back("Tag " + std::to_string(tagCode) + " truncated (length " + std::to_string(tagLength) + " exceeds remaining " + std::to_string(reader.remaining()) + ")"); break; }
        const uint8_t* payload = reader.currentPtr();
        try {
            switch (static_cast<TagCode>(tagCode)) {
                case TagCode::End: reader.skip(tagLength); goto parse_finished;
                case TagCode::FileAttributes:
                    if (tagLength >= 4) { const uint32_t flags = static_cast<uint32_t>(payload[0]) | (static_cast<uint32_t>(payload[1]) << 8) | (static_cast<uint32_t>(payload[2]) << 16) | (static_cast<uint32_t>(payload[3]) << 24); m_metadata.isActionScript3 = (flags & (1 << 3)) != 0; } break;
                case TagCode::SymbolClass: handleSymbolClass(payload, tagLength); break;
                case TagCode::DefineBitsJPEG2: handleDefineBitsJPEG2(payload, tagLength); break;
                case TagCode::DefineBitsJPEG3: case TagCode::DefineBitsJPEG4: handleDefineBitsJPEG3(payload, tagLength); break;
                case TagCode::DefineBitsLossless: handleDefineBitsLossless(payload, tagLength, false); break;
                case TagCode::DefineBitsLossless2: handleDefineBitsLossless(payload, tagLength, true); break;
                case TagCode::DefineShape: case TagCode::DefineShape2: case TagCode::DefineShape3: case TagCode::DefineShape4: handleDefineShape(payload, tagLength, tagCode); break;
                case TagCode::DefineSprite: handleDefineSprite(payload, tagLength); break;
                case TagCode::DefineSound: handleDefineSound(payload, tagLength); break;
                case TagCode::DoABC: handleDoABC(payload, tagLength); break;
                default: break;
            }
        } catch (const std::exception& e) { m_warnings.push_back("Failed parsing SWF tag " + std::to_string(tagCode) + ": " + e.what()); }
        reader.skip(tagLength); ++tagIndex;
        if (progress && (tagIndex % 50 == 0)) { const float pct = 0.1f + 0.8f * (static_cast<float>(reader.position()) / static_cast<float>(tagSize)); progress(pct, "Parsing SWF tags (" + std::to_string(m_images.size()) + " textures, " + std::to_string(m_sounds.size()) + " sounds)..."); }
    }
parse_finished:
    for (auto& img : m_images) img.className = findSymbolName(img.characterId);
    for (auto& snd : m_sounds) snd.className = findSymbolName(snd.characterId);
    resolveSymbolArtwork();
    m_metadata.imageCount = static_cast<uint32_t>(m_images.size()); m_metadata.soundCount = static_cast<uint32_t>(m_sounds.size()); m_metadata.symbolCount = static_cast<uint32_t>(m_characterToSymbol.size());
    for (const auto& [cid, name] : m_characterToSymbol) { (void)cid; m_metadata.symbolNames.push_back(name); }
    if (progress) progress(1.0f, "SWF parse complete.");
    return true;
}

void SwfParser::handleSymbolClass(const uint8_t* payload, size_t length) {
    try {
        SwfReader r(payload, length); const uint16_t numSymbols = r.readUI16();
        for (uint16_t i = 0; i < numSymbols; ++i) { if (r.remaining() < 3) break; const uint16_t characterId = r.readUI16(); const std::string className = r.readString(); m_characterToSymbol[characterId] = className; m_symbolToCharacter[className] = characterId; }
    } catch (...) { m_warnings.push_back("Failed parsing SymbolClass tag."); }
}

void SwfParser::handleDefineBitsJPEG2(const uint8_t* payload, size_t length) {
    if (length < 4) return; const uint16_t characterId = static_cast<uint16_t>(payload[0]) | (static_cast<uint16_t>(payload[1]) << 8); SwfImage img; img.characterId = characterId; img.format = ImageFormat::JPEG; img.data.assign(payload + 2, payload + length); m_images.push_back(std::move(img));
}

void SwfParser::handleDefineBitsJPEG3(const uint8_t* payload, size_t length) {
    if (length < 6) return; const uint16_t characterId = static_cast<uint16_t>(payload[0]) | (static_cast<uint16_t>(payload[1]) << 8); const uint32_t alphaOffset = static_cast<uint32_t>(payload[2]) | (static_cast<uint32_t>(payload[3]) << 8) | (static_cast<uint32_t>(payload[4]) << 16) | (static_cast<uint32_t>(payload[5]) << 24); if (6 + alphaOffset > length) return; SwfImage img; img.characterId = characterId; img.format = ImageFormat::JPEG; img.data.assign(payload + 6, payload + 6 + alphaOffset); m_images.push_back(std::move(img));
}

void SwfParser::handleDefineBitsLossless(const uint8_t* payload, size_t length, bool isVersion2) {
    if (length < 7) return;
    const uint16_t characterId = static_cast<uint16_t>(payload[0]) | (static_cast<uint16_t>(payload[1]) << 8);
    const uint8_t bitmapFormat = payload[2];
    const uint16_t width = static_cast<uint16_t>(payload[3]) | (static_cast<uint16_t>(payload[4]) << 8);
    const uint16_t height = static_cast<uint16_t>(payload[5]) | (static_cast<uint16_t>(payload[6]) << 8);
    const uint8_t* zlibData = payload + 7;
    const size_t zlibSize = length - 7;
    std::vector<uint8_t> compressed(zlibData, zlibData + zlibSize);
    std::vector<uint8_t> rgba;
    if (!decodeLosslessToRgba(compressed, bitmapFormat, width, height, isVersion2, rgba)) {
        m_warnings.push_back("Unsupported or invalid DefineBitsLossless" + std::string(isVersion2 ? "2" : "") +
                             " bitmap format " + std::to_string(bitmapFormat) + " for character " + std::to_string(characterId));
        return;
    }
    SwfImage img;
    img.characterId = characterId;
    img.width = width;
    img.height = height;
    img.format = ImageFormat::RGBA;
    img.data = std::move(rgba);
    m_images.push_back(std::move(img));
}

void SwfParser::handleDefineShape(const uint8_t* payload, size_t length, uint16_t tagCode) {
    if (length < 4) return; const uint16_t characterId = static_cast<uint16_t>(payload[0]) | (static_cast<uint16_t>(payload[1]) << 8); std::vector<uint16_t> refs = extractShapeBitmapRefs(payload, length, tagCode); auto& target = m_characterBitmapRefs[characterId]; for (uint16_t ref : refs) appendUnique(target, ref);
}

void SwfParser::handleDefineSprite(const uint8_t* payload, size_t length) {
    if (length < 4) return; const uint16_t spriteId = static_cast<uint16_t>(payload[0]) | (static_cast<uint16_t>(payload[1]) << 8); const uint16_t frameCount = static_cast<uint16_t>(payload[2]) | (static_cast<uint16_t>(payload[3]) << 8); (void)frameCount;
    if (length == 4) return;
    SwfReader reader(payload + 4, length - 4);
    while (!reader.isEof() && reader.remaining() >= 2) {
        const uint16_t tagWord = reader.readUI16(); const uint16_t tagCode = tagWord >> 6; uint32_t tagLength = tagWord & 0x3F;
        if (tagLength == 0x3F) { if (reader.remaining() < 4) break; tagLength = reader.readUI32(); }
        if (reader.remaining() < tagLength) break;
        const uint8_t* nested = reader.currentPtr();
        try {
            if (tagCode == 4 && tagLength >= 4) { appendUnique(m_characterChildren[spriteId], static_cast<uint16_t>(nested[0]) | (static_cast<uint16_t>(nested[1]) << 8)); }
            else if (tagCode == 26 && tagLength >= 4) { const uint8_t flags = nested[0]; if (flags & 0x02) appendUnique(m_characterChildren[spriteId], static_cast<uint16_t>(nested[3]) | (static_cast<uint16_t>(nested[4]) << 8)); }
            else if (tagCode == 70 && tagLength >= 5) { const uint8_t flags1 = nested[0]; const uint8_t flags2 = nested[1]; size_t pos = 2; if (flags2 & 0x08) { SwfReader classReader(nested + pos, tagLength - pos); (void)classReader.readString(); pos += classReader.position(); } if ((flags1 & 0x02) && pos + 2 <= tagLength) appendUnique(m_characterChildren[spriteId], static_cast<uint16_t>(nested[pos]) | (static_cast<uint16_t>(nested[pos + 1]) << 8)); }
        } catch (const std::exception& e) { m_warnings.push_back("Failed parsing DefineSprite child: " + std::string(e.what())); }
        reader.skip(tagLength);
        if (tagCode == 0) break;
    }
}

bool SwfParser::resolveSymbolArtwork() {
    std::unordered_set<uint16_t> recursionGuard;
    std::function<void(uint16_t, std::unordered_set<uint16_t>&)> collect = [&](uint16_t id, std::unordered_set<uint16_t>& output) {
        if (!recursionGuard.insert(id).second) return;
        auto bitmapIt = m_characterBitmapRefs.find(id);
        if (bitmapIt != m_characterBitmapRefs.end()) for (uint16_t bitmapId : bitmapIt->second) output.insert(bitmapId);
        auto childIt = m_characterChildren.find(id);
        if (childIt != m_characterChildren.end()) for (uint16_t childId : childIt->second) collect(childId, output);
        recursionGuard.erase(id);
    };
    std::vector<std::pair<uint16_t, std::string>> symbols;
    for (const auto& [id, name] : m_characterToSymbol) symbols.emplace_back(id, name);
    for (auto& image : m_images) {
        if (!image.className.empty()) continue;
        for (const auto& [symbolId, className] : symbols) {
            std::unordered_set<uint16_t> bitmaps;
            collect(symbolId, bitmaps);
            if (bitmaps.count(image.characterId)) { image.className = className; break; }
        }
    }
    return true;
}

void SwfParser::handleDefineSound(const uint8_t* payload, size_t length) {
    if (length < 7) return;
    const uint16_t characterId = static_cast<uint16_t>(payload[0]) | (static_cast<uint16_t>(payload[1]) << 8); const uint8_t flags = payload[2]; const uint8_t formatCode = (flags >> 4) & 0x0F; const uint8_t rateCode = (flags >> 2) & 0x03; const bool is16Bit = (flags & 0x02) != 0; const bool isStereo = (flags & 0x01) != 0; const uint32_t sampleCount = static_cast<uint32_t>(payload[3]) | (static_cast<uint32_t>(payload[4]) << 8) | (static_cast<uint32_t>(payload[5]) << 16) | (static_cast<uint32_t>(payload[6]) << 24);
    SwfSound snd; snd.characterId = characterId; snd.is16Bit = is16Bit; snd.isStereo = isStereo; snd.sampleCount = sampleCount;
    switch (rateCode) { case 0: snd.sampleRate = 5512; break; case 1: snd.sampleRate = 11025; break; case 2: snd.sampleRate = 22050; break; case 3: snd.sampleRate = 44100; break; }
    switch (formatCode) { case 0: snd.format = SoundFormat::RawPCM; break; case 1: snd.format = SoundFormat::ADPCM; break; case 2: snd.format = SoundFormat::MP3; break; case 3: snd.format = SoundFormat::RawLittleEndianPCM; break; case 6: snd.format = SoundFormat::Nellymoser; break; case 11: snd.format = SoundFormat::Speex; break; default: snd.format = SoundFormat::MP3; break; }
    if (length > 7) snd.data.assign(payload + 7, payload + length); m_sounds.push_back(std::move(snd));
}

void SwfParser::handleDoABC(const uint8_t* payload, size_t length) { (void)payload; (void)length; }

} // namespace btd4::swf
