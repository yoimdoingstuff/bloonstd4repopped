#pragma once

#include "../swf/SwfTypes.hpp"
#include <string>
#include <vector>

namespace btd4::tools {

class AssetConverter {
public:
    // Write image to disk (JPEG / PNG / BMP)
    static bool saveImage(const swf::SwfImage& image, const std::string& outputPath);

    // Write audio to disk (WAV / MP3)
    static bool saveSound(const swf::SwfSound& sound, const std::string& outputPath);

    // Converts symbol class names to normalized asset IDs
    static std::string normalizeIdentifier(const std::string& rawName, const std::string& fallbackPrefix, uint16_t id);
};

} // namespace btd4::tools
