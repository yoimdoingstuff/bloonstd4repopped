#include "AssetImporter.hpp"
#include "../swf/SwfParser.hpp"
#include "../asset_converter/AssetConverter.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

namespace btd4::tools {

namespace fs = std::filesystem;

namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool containsAny(const std::string& value, const std::initializer_list<const char*>& needles) {
    const std::string normalized = lower(value);
    for (const char* needle : needles) {
        if (normalized.find(needle) != std::string::npos) return true;
    }
    return false;
}

bool looksLikeIpaArchive(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    unsigned char signature[4]{};
    file.read(reinterpret_cast<char*>(signature), sizeof(signature));
    if (file.gcount() != 4) return false;
    return signature[0] == 'P' && signature[1] == 'K' &&
           signature[2] == 0x03 && signature[3] == 0x04;
}

void inspectIpa(const std::string& path, ImportReport& report,
                const std::function<void(const std::string&)>& emitLog) {
    if (path.empty() || !fs::exists(path)) return;

    report.ipaDetected = true;
    report.ipaArchiveDetected = looksLikeIpaArchive(path);
    if (!report.ipaArchiveDetected) {
        report.warnings.push_back("IPA path exists but does not have a standard ZIP/IPA signature.");
        emitLog("[Importer] IPA supplied, but archive signature was not recognized.");
        return;
    }

    report.detectedFeatures.push_back("IPA archive");
    emitLog("[Importer] IPA archive detected.");

    // Lightweight resource discovery deliberately avoids depending on a second
    // ZIP library. This scans archive bytes for common path markers while the
    // full IPA extraction step remains a separate importer task.
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return;
    const std::streamoff size = file.tellg();
    if (size <= 0 || size > static_cast<std::streamoff>(256 * 1024 * 1024)) {
        report.warnings.push_back("IPA resource scan skipped because the archive is empty or exceeds the 256 MiB safety limit.");
        return;
    }
    file.seekg(0, std::ios::beg);
    std::string bytes(static_cast<size_t>(size), '\0');
    file.read(bytes.data(), size);
    if (!file) return;

    const std::string normalized = lower(bytes);
    if (normalized.find("info.plist") != std::string::npos) {
        report.detectedFeatures.push_back("Info.plist");
        emitLog("[Importer] IPA contains Info.plist resources.");
    }
    if (normalized.find("beekeeper") != std::string::npos) {
        report.detectedFeatures.push_back("Beekeeper candidate");
        emitLog("[Importer] Found Beekeeper-related resource names in IPA index data.");
    }
    if (normalized.find("payload/") != std::string::npos) {
        report.detectedFeatures.push_back("Payload bundle");
    }
}

std::string detectSourceFamily(const swf::SwfParser& parser, bool& isBtd4,
                               std::vector<std::string>& features) {
    int score = 0;
    int gameplaySymbols = 0;

    for (const auto& symbol : parser.symbols()) {
        const std::string name = lower(symbol.className);
        if (containsAny(name, {"bloon", "monkey", "tower", "dart", "tack", "boomerang", "sniper", "bomb"})) {
            ++gameplaySymbols;
            ++score;
        }
        if (name.find("bloon") != std::string::npos) features.push_back("Bloon symbols");
        if (name.find("tower") != std::string::npos || name.find("monkey") != std::string::npos) {
            features.push_back("Tower symbols");
        }
        if (name.find("map") != std::string::npos) features.push_back("Map symbols");
    }

    for (const auto& image : parser.images()) {
        const std::string name = lower(image.className);
        if (containsAny(name, {"bloon", "monkey", "tower", "dart", "tack", "boomerang", "sniper", "bomb"})) {
            ++score;
        }
    }

    std::sort(features.begin(), features.end());
    features.erase(std::unique(features.begin(), features.end()), features.end());

    isBtd4 = gameplaySymbols >= 3 && score >= 5;
    return isBtd4 ? "BloonsTD4Flash" : "UnknownSWF";
}

} // namespace

ImportReport AssetImporter::run(const ImportOptions& options,
                                LogCallback logCallback,
                                ProgressCallback progressCallback) {
    ImportReport report;
    report.sourceFile = options.sourceSwf;
    report.targetPlatform = options.targetPlatform;

    auto emitLog = [&](const std::string& msg) {
        report.logMessages.push_back(msg);
        if (logCallback) logCallback(msg);
    };

    emitLog("[Importer] Initializing BTD4 Asset Import Pipeline...");
    emitLog("[Importer] Target platform: " + options.targetPlatform);

    if (options.sourceSwf.empty()) {
        report.success = false;
        report.errorMessage = "No source SWF file specified";
        emitLog("[Importer Error] " + report.errorMessage);
        return report;
    }

    if (!fs::exists(options.sourceSwf)) {
        report.success = false;
        report.errorMessage = "Source file does not exist: " + options.sourceSwf;
        emitLog("[Importer Error] " + report.errorMessage);
        return report;
    }

    emitLog("[Importer] Inspecting source: " + options.sourceSwf);

    swf::SwfParser parser;
    std::string parseError;

    auto swfProgress = [&](float p, const std::string& status) {
        if (progressCallback) progressCallback(p * 0.5f, status);
    };

    if (!parser.parseFile(options.sourceSwf, parseError, swfProgress)) {
        report.success = false;
        report.errorMessage = "SWF parsing failed: " + parseError;
        emitLog("[Importer Error] " + report.errorMessage);
        return report;
    }

    report.swfVersion = parser.header().version;
    report.symbolsMapped = static_cast<uint32_t>(parser.symbols().size());
    report.warnings = parser.warnings();

    emitLog("[Importer] SWF Header parsed successfully (Version: " + std::to_string(report.swfVersion) +
            ", Size: " + std::to_string(parser.header().frameSize.widthPixels()) + "x" +
            std::to_string(parser.header().frameSize.heightPixels()) + ")");
    emitLog("[Importer] Found " + std::to_string(parser.images().size()) + " images, " +
            std::to_string(parser.sounds().size()) + " audio streams, " +
            std::to_string(report.symbolsMapped) + " exported symbols.");

    report.sourceFamily = detectSourceFamily(parser, report.btd4Detected, report.detectedFeatures);
    if (report.btd4Detected) {
        emitLog("[Importer] BTD4-like Flash content detected from exported gameplay symbols/assets.");
    } else {
        emitLog("[Importer] No confident BTD4 Flash signature was detected; continuing in generic SWF mode.");
    }

    inspectIpa(options.sourceIpa, report, emitLog);

    if (options.targetPlatform == "PSP") {
        report.detectedFeatures.push_back("PSP asset profile");
        emitLog("[Importer] PSP profile enabled: preserving source textures while keeping the runtime at 480x272 logical coordinates.");
    } else if (options.targetPlatform == "Windows" || options.targetPlatform == "Linux") {
        report.detectedFeatures.push_back("Desktop asset profile");
        emitLog("[Importer] Desktop profile enabled: using source-resolution assets with logical-resolution scaling in the runtime.");
    } else if (options.targetPlatform == "Xbox 360") {
        report.detectedFeatures.push_back("Xbox 360 asset profile");
        emitLog("[Importer] Xbox 360 profile selected; platform packaging remains dependent on the available backend/toolchain.");
    }

    std::sort(report.detectedFeatures.begin(), report.detectedFeatures.end());
    report.detectedFeatures.erase(std::unique(report.detectedFeatures.begin(), report.detectedFeatures.end()), report.detectedFeatures.end());

    fs::path outDir = options.outputDir;
    fs::path texturesDir = outDir / "textures";
    fs::path audioDir = outDir / "audio";
    fs::path mapsDir = outDir / "maps";
    fs::path roundsDir = outDir / "rounds";

    try {
        fs::create_directories(texturesDir);
        fs::create_directories(audioDir);
        fs::create_directories(mapsDir);
        fs::create_directories(roundsDir);
    } catch (const std::exception& e) {
        report.success = false;
        report.errorMessage = std::string("Failed to create output directory: ") + e.what();
        emitLog("[Importer Error] " + report.errorMessage);
        return report;
    }

    std::vector<std::string> textureManifestEntries;
    if (options.extractTextures) {
        emitLog("[Importer] Converting textures...");
        const size_t totalImages = parser.images().size();
        for (size_t i = 0; i < totalImages; ++i) {
            const auto& img = parser.images()[i];
            std::string baseId = AssetConverter::normalizeIdentifier(img.className, "tex", img.characterId);
            std::string ext = (img.format == swf::ImageFormat::JPEG) ? ".jpg" : ".bmp";
            std::string fileName = baseId + ext;
            fs::path filePath = texturesDir / fileName;

            if (AssetConverter::saveImage(img, filePath.string())) {
                report.texturesExtracted++;
                textureManifestEntries.push_back("    \"" + baseId + "\": \"textures/" + fileName + "\"");
            }

            if (progressCallback && (i % 20 == 0 || i + 1 == totalImages)) {
                float p = totalImages == 0 ? 0.8f : 0.5f + 0.3f * (static_cast<float>(i + 1) / static_cast<float>(totalImages));
                progressCallback(p, "Exporting textures (" + std::to_string(i + 1) + "/" + std::to_string(totalImages) + ")...");
            }
        }
        emitLog("[Importer] Extracted " + std::to_string(report.texturesExtracted) + " textures.");
    }

    std::vector<std::string> audioManifestEntries;
    if (options.extractAudio) {
        emitLog("[Importer] Converting audio...");
        const size_t totalSounds = parser.sounds().size();
        for (size_t i = 0; i < totalSounds; ++i) {
            const auto& snd = parser.sounds()[i];
            std::string baseId = AssetConverter::normalizeIdentifier(snd.className, "snd", snd.characterId);
            std::string ext = (snd.format == swf::SoundFormat::MP3) ? ".mp3" : ".wav";
            std::string fileName = baseId + ext;
            fs::path filePath = audioDir / fileName;

            if (AssetConverter::saveSound(snd, filePath.string())) {
                report.soundsExtracted++;
                audioManifestEntries.push_back("    \"" + baseId + "\": \"audio/" + fileName + "\"");
            }

            if (progressCallback && (i % 10 == 0 || i + 1 == totalSounds)) {
                float p = totalSounds == 0 ? 0.95f : 0.8f + 0.15f * (static_cast<float>(i + 1) / static_cast<float>(totalSounds));
                progressCallback(p, "Exporting audio (" + std::to_string(i + 1) + "/" + std::to_string(totalSounds) + ")...");
            }
        }
        emitLog("[Importer] Extracted " + std::to_string(report.soundsExtracted) + " audio cues.");
    }

    if (options.generateManifest) {
        emitLog("[Importer] Generating manifest.json...");
        fs::path manifestFile = outDir / "manifest.json";
        std::ofstream mf(manifestFile);
        if (!mf.is_open()) {
            report.success = false;
            report.errorMessage = "Unable to create manifest: " + manifestFile.string();
            emitLog("[Importer Error] " + report.errorMessage);
            return report;
        }

        mf << "{\n";
        mf << "  \"version\": 2,\n";
        mf << "  \"package_name\": \"Bloons TD 4 Game Data\",\n";
        mf << "  \"source\": \"" << options.sourceSwf << "\",\n";
        mf << "  \"source_family\": \"" << report.sourceFamily << "\",\n";
        mf << "  \"btd4_detected\": " << (report.btd4Detected ? "true" : "false") << ",\n";
        mf << "  \"swf_version\": " << static_cast<unsigned>(report.swfVersion) << ",\n";
        mf << "  \"target_platform\": \"" << options.targetPlatform << "\",\n";
        mf << "  \"ipa_detected\": " << (report.ipaDetected ? "true" : "false") << ",\n";
        mf << "  \"ipa_archive_detected\": " << (report.ipaArchiveDetected ? "true" : "false") << ",\n";
        mf << "  \"detected_features\": [\n";
        for (size_t i = 0; i < report.detectedFeatures.size(); ++i) {
            mf << "    \"" << report.detectedFeatures[i] << "\"";
            if (i + 1 < report.detectedFeatures.size()) mf << ',';
            mf << "\n";
        }
        mf << "  ],\n";
        mf << "  \"textures\": {\n";
        for (size_t i = 0; i < textureManifestEntries.size(); ++i) {
            mf << textureManifestEntries[i];
            if (i + 1 < textureManifestEntries.size()) mf << ',';
            mf << "\n";
        }
        mf << "  },\n";
        mf << "  \"audio\": {\n";
        for (size_t i = 0; i < audioManifestEntries.size(); ++i) {
            mf << audioManifestEntries[i];
            if (i + 1 < audioManifestEntries.size()) mf << ',';
            mf << "\n";
        }
        mf << "  },\n";
        mf << "  \"maps\": [\"maps/original_map.json\"],\n";
        mf << "  \"rounds\": \"rounds/default_rounds.json\"\n";
        mf << "}\n";
        mf.close();

        report.manifestPath = manifestFile.string();
        emitLog("[Importer] Manifest saved to: " + report.manifestPath);
    }

    if (progressCallback) progressCallback(1.0f, "Import pipeline finished!");

    report.success = true;
    emitLog("[Importer Success] Asset pipeline finished successfully. Ready for " + options.targetPlatform + ".");
    return report;
}

} // namespace btd4::tools
