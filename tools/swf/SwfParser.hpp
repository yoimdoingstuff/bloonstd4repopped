#pragma once

#include "SwfTypes.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace btd4::swf {

class SwfParser {
public:
    using ProgressCallback = std::function<void(float progress, const std::string& status)>;

    SwfParser();

    bool parse(const uint8_t* data, size_t size, std::string& outError, ProgressCallback progress = nullptr);
    bool parseFile(const std::string& filePath, std::string& outError, ProgressCallback progress = nullptr);

    const SwfHeader& header() const { return m_header; }
    const SwfMetadata& metadata() const { return m_metadata; }

    const std::vector<SwfImage>& images() const { return m_images; }
    const std::vector<SwfSound>& sounds() const { return m_sounds; }
    const std::unordered_map<uint16_t, std::string>& symbols() const { return m_characterToSymbol; }
    const std::vector<std::string>& warnings() const { return m_warnings; }

    // Look up symbol class name for character ID
    std::string findSymbolName(uint16_t characterId) const;
    uint16_t findCharacterId(const std::string& className) const;

private:
    SwfHeader m_header;
    SwfMetadata m_metadata;

    std::vector<SwfImage> m_images;
    std::vector<SwfSound> m_sounds;
    std::unordered_map<uint16_t, std::string> m_characterToSymbol;
    std::unordered_map<std::string, uint16_t> m_symbolToCharacter;
    std::unordered_map<uint16_t, std::vector<uint16_t>> m_characterBitmapRefs;
    std::unordered_map<uint16_t, std::vector<uint16_t>> m_characterChildren;
    std::vector<std::string> m_warnings;

    bool parseTags(const uint8_t* tagData, size_t tagSize, std::string& outError, ProgressCallback progress);
    void handleSymbolClass(const uint8_t* payload, size_t length);
    void handleDefineBitsJPEG2(const uint8_t* payload, size_t length);
    void handleDefineBitsJPEG3(const uint8_t* payload, size_t length);
    void handleDefineBitsLossless(const uint8_t* payload, size_t length, bool isVersion2);
    void handleDefineShape(const uint8_t* payload, size_t length, uint16_t tagCode);
    void handleDefineSprite(const uint8_t* payload, size_t length);
    void handleDefineSound(const uint8_t* payload, size_t length);
    void handleDoABC(const uint8_t* payload, size_t length);
    bool resolveSymbolArtwork();
};

} // namespace btd4::swf
