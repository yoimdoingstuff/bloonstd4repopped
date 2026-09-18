#pragma once

#include "../swf/SwfTypes.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace btd4::tools {

struct ImportOptions {
    std::string sourceSwf;
    std::string sourceIpa;
    std::string outputDir{"game_data"};
    std::string targetPlatform{"auto"};
    bool extractTextures{true};
    bool extractAudio{true};
    bool generateManifest{true};
};

struct ImportReport {
    bool success{false};
    std::string errorMessage;
    std::string sourceFile;
    uint8_t swfVersion{0};
    uint32_t texturesExtracted{0};
    uint32_t soundsExtracted{0};
    uint32_t symbolsMapped{0};
    bool btd4Detected{false};
    bool ipaDetected{false};
    bool ipaArchiveDetected{false};
    uint32_t ipaFilesExtracted{0};
    uint64_t ipaBytesExtracted{0};
    std::string ipaOutputDirectory;
    std::string sourceFamily{"Unknown"};
    std::string targetPlatform{"auto"};
    std::vector<std::string> detectedFeatures;
    std::string manifestPath;
    std::vector<std::string> warnings;
    std::vector<std::string> logMessages;
};

class AssetImporter {
public:
    using LogCallback = std::function<void(const std::string& message)>;
    using ProgressCallback = std::function<void(float progress, const std::string& status)>;

    static ImportReport run(const ImportOptions& options,
                            LogCallback logCallback = nullptr,
                            ProgressCallback progressCallback = nullptr);
};

} // namespace btd4::tools
