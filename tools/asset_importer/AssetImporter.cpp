#include "AssetImporter.hpp"
#include "../swf/SwfParser.hpp"
#include "../asset_converter/AssetConverter.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

namespace btd4::tools {

namespace fs = std::filesystem;

ImportReport AssetImporter::run(const ImportOptions& options,
                                LogCallback logCallback,
                                ProgressCallback progressCallback) {
    ImportReport report;
    report.sourceFile = options.sourceSwf;

    auto emitLog = [&](const std::string& msg) {
        report.logMessages.push_back(msg);
        if (logCallback) logCallback(msg);
    };

    emitLog("[Importer] Initializing BTD4 Asset Import Pipeline...");

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

    // 1. Parse SWF
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

    // 2. Prepare Output Directories
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

    // 3. Extract & Convert Textures
    std::vector<std::string> textureManifestEntries;
    if (options.extractTextures) {
        emitLog("[Importer] Converting textures...");
        size_t totalImages = parser.images().size();
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

            if (progressCallback && (i % 20 == 0 || i == totalImages - 1)) {
                float p = 0.5f + 0.3f * (static_cast<float>(i + 1) / static_cast<float>(totalImages));
                progressCallback(p, "Exporting textures (" + std::to_string(i + 1) + "/" + std::to_string(totalImages) + ")...");
            }
        }
        emitLog("[Importer] Extracted " + std::to_string(report.texturesExtracted) + " textures.");
    }

    // 4. Extract & Convert Audio
    std::vector<std::string> audioManifestEntries;
    if (options.extractAudio) {
        emitLog("[Importer] Converting audio...");
        size_t totalSounds = parser.sounds().size();
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

            if (progressCallback && (i % 10 == 0 || i == totalSounds - 1)) {
                float p = 0.8f + 0.15f * (static_cast<float>(i + 1) / static_cast<float>(totalSounds));
                progressCallback(p, "Exporting audio (" + std::to_string(i + 1) + "/" + std::to_string(totalSounds) + ")...");
            }
        }
        emitLog("[Importer] Extracted " + std::to_string(report.soundsExtracted) + " audio cues.");
    }

    // 5. Generate manifest.json
    if (options.generateManifest) {
        emitLog("[Importer] Generating manifest.json...");
        fs::path manifestFile = outDir / "manifest.json";
        std::ofstream mf(manifestFile);
        if (mf.is_open()) {
            mf << "{\n";
            mf << "  \"version\": 1,\n";
            mf << "  \"package_name\": \"Bloons TD 4 Game Data\",\n";
            mf << "  \"source\": \"" << options.sourceSwf << "\",\n";
            mf << "  \"textures\": {\n";
            for (size_t i = 0; i < textureManifestEntries.size(); ++i) {
                mf << textureManifestEntries[i];
                if (i + 1 < textureManifestEntries.size()) mf << ",";
                mf << "\n";
            }
            mf << "  },\n";
            mf << "  \"audio\": {\n";
            for (size_t i = 0; i < audioManifestEntries.size(); ++i) {
                mf << audioManifestEntries[i];
                if (i + 1 < audioManifestEntries.size()) mf << ",";
                mf << "\n";
            }
            mf << "  },\n";
            mf << "  \"maps\": [\n";
            mf << "    \"maps/original_map.json\"\n";
            mf << "  ],\n";
            mf << "  \"rounds\": \"rounds/default_rounds.json\"\n";
            mf << "}\n";
            mf.close();

            report.manifestPath = manifestFile.string();
            emitLog("[Importer] Manifest saved to: " + report.manifestPath);
        }
    }

    if (progressCallback) progressCallback(1.0f, "Import pipeline finished!");

    report.success = true;
    emitLog("[Importer Success] Asset pipeline finished successfully. Ready for engine.");
    return report;
}

} // namespace btd4::tools
