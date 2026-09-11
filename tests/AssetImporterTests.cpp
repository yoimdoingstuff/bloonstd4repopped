#include "TestRunner.hpp"
#include "../tools/swf/SwfTypes.hpp"
#include "../tools/swf/SwfReader.hpp"
#include "../tools/swf/Inflate.hpp"
#include "../tools/swf/SwfParser.hpp"
#include "../tools/asset_converter/AssetConverter.hpp"
#include "../tools/asset_importer/AssetImporter.hpp"
#include "../engine/assets/AssetManifest.hpp"
#include "../engine/assets/AssetManager.hpp"
#include "../platform/common/NativeFileSystem.hpp"
#include <vector>
#include <string>

// 1. Bitstream and Rect parsing
TEST_CASE(SwfReaderBitstreamParsing) {
    // Rect with 5 bits per field:
    // Nbits = 5 (00101)
    // xMin = 0 (00000)
    // xMax = 10 (01010)
    // yMin = 0 (00000)
    // yMax = 10 (01010)
    // Total bits = 5 + 4*5 = 25 bits
    // Packed: [00101000] [00010100] [00000101] [00000000]
    std::vector<uint8_t> buffer = {
        0x28, 0x14, 0x05, 0x00
    };

    btd4::swf::SwfReader reader(buffer);
    btd4::swf::SwfRect rect = reader.readRect();
    TEST_ASSERT_EQ(rect.xMinTwips, 0);
    TEST_ASSERT_EQ(rect.xMaxTwips, 10);
    TEST_ASSERT_EQ(rect.yMinTwips, 0);
    TEST_ASSERT_EQ(rect.yMaxTwips, 10);
}

// 2. Uncompressed DEFLATE decompression
TEST_CASE(InflateUncompressedBlock) {
    // DEFLATE uncompressed block:
    // BFINAL = 1, BTYPE = 00 -> 0x01
    // LEN = 4 (0x04, 0x00)
    // NLEN = ~4 (0xFB, 0xFF)
    // Payload: 'T', 'E', 'S', 'T'
    std::vector<uint8_t> deflateData = {
        0x01, 0x04, 0x00, 0xFB, 0xFF, 'T', 'E', 'S', 'T'
    };

    std::vector<uint8_t> decompressed;
    bool ok = btd4::swf::Inflate::decompressDeflate(deflateData.data(), deflateData.size(), decompressed);
    TEST_ASSERT(ok);
    TEST_ASSERT_EQ(decompressed.size(), static_cast<size_t>(4));
    std::string text(decompressed.begin(), decompressed.end());
    TEST_ASSERT_EQ(text, "TEST");
}

// 3. Synthetic FWS SWF header & tag parsing
TEST_CASE(SwfParserSyntheticFWS) {
    std::vector<uint8_t> swf;

    // Header: 'FWS', Version 10, FileLength 34
    swf.push_back('F'); swf.push_back('W'); swf.push_back('S');
    swf.push_back(10);
    uint32_t fileLen = 34;
    swf.push_back(fileLen & 0xFF);
    swf.push_back((fileLen >> 8) & 0xFF);
    swf.push_back((fileLen >> 16) & 0xFF);
    swf.push_back((fileLen >> 24) & 0xFF);

    // Frame size rect: 5 bits per field (0, 9600, 0, 5440 twips = 480x272 px)
    // 0x78, 0x00, 0x25, 0x80, 0x00, 0x15, 0x40
    // Simplified rect:
    swf.push_back(0x28); swf.push_back(0x05); swf.push_back(0x00); swf.push_back(0x50);

    // Frame rate: 60.0 fps (60 << 8 = 15360 = 0x3C00)
    swf.push_back(0x00); swf.push_back(0x3C);

    // Frame count: 1
    swf.push_back(0x01); swf.push_back(0x00);

    // Tag 1: FileAttributes (TagCode 69 = 0x45, length 4)
    // Tag header: (69 << 6) | 4 = 0x1144 -> 0x44, 0x11
    swf.push_back(0x44); swf.push_back(0x11);
    // Flags: ActionScript 3 (bit 3) = 0x08, 0x00, 0x00, 0x00
    swf.push_back(0x08); swf.push_back(0x00); swf.push_back(0x00); swf.push_back(0x00);

    // Tag 2: SymbolClass (TagCode 76 = 0x4C)
    // Payload: numSymbols = 1, charId = 10, name = "MonkeyDart" + '\0'
    std::string className = "MonkeyDart";
    uint32_t symLength = 2 + 2 + className.size() + 1;
    uint16_t symTagHeader = (76 << 6) | (symLength & 0x3F);
    swf.push_back(symTagHeader & 0xFF);
    swf.push_back((symTagHeader >> 8) & 0xFF);
    swf.push_back(0x01); swf.push_back(0x00); // 1 symbol
    swf.push_back(10); swf.push_back(0);      // charId = 10
    for (char c : className) swf.push_back(static_cast<uint8_t>(c));
    swf.push_back(0); // null terminator

    // Tag 3: End Tag (TagCode 0, length 0) -> 0x00, 0x00
    swf.push_back(0x00); swf.push_back(0x00);

    btd4::swf::SwfParser parser;
    std::string err;
    bool ok = parser.parse(swf.data(), swf.size(), err);
    TEST_ASSERT(ok);
    TEST_ASSERT_EQ(parser.header().version, 10);
    TEST_ASSERT(parser.metadata().isActionScript3);

    // Check SymbolClass lookup
    std::string sym = parser.findSymbolName(10);
    TEST_ASSERT_EQ(sym, "MonkeyDart");
    TEST_ASSERT_EQ(parser.findCharacterId("MonkeyDart"), 10);
}

// 4. Asset Converter Identifier Normalization
TEST_CASE(AssetConverterNormalization) {
    std::string norm1 = btd4::tools::AssetConverter::normalizeIdentifier("MonkeyDart", "tex", 1);
    TEST_ASSERT_EQ(norm1, "monkey_dart");

    std::string norm2 = btd4::tools::AssetConverter::normalizeIdentifier("TackTower_level2", "tex", 2);
    TEST_ASSERT_EQ(norm2, "tack_tower_level2");

    std::string norm3 = btd4::tools::AssetConverter::normalizeIdentifier("", "fallback", 42);
    TEST_ASSERT_EQ(norm3, "fallback_42");
}

// 5. Asset Manifest Serialization and Loading
TEST_CASE(AssetManifestSerializationAndParsing) {
    btd4::AssetManifest manifest;
    manifest.packageName = "Test Package";
    manifest.source = "game.swf";
    manifest.textures["dart_monkey"] = "textures/dart_monkey.bmp";
    manifest.audio["pop"] = "audio/pop.wav";
    manifest.maps.push_back("maps/level1.json");
    manifest.roundsFile = "rounds/rounds.json";

    std::string json = manifest.serialize();
    TEST_ASSERT(json.find("\"package_name\": \"Test Package\"") != std::string::npos);
    TEST_ASSERT(json.find("\"dart_monkey\": \"textures/dart_monkey.bmp\"") != std::string::npos);

    btd4::AssetManifest parsed;
    std::string err;
    bool ok = parsed.parseJson(json, err);
    TEST_ASSERT(ok);
    TEST_ASSERT_EQ(parsed.packageName, "Test Package");
    TEST_ASSERT_EQ(parsed.textures["dart_monkey"], "textures/dart_monkey.bmp");
    TEST_ASSERT_EQ(parsed.audio["pop"], "audio/pop.wav");
    TEST_ASSERT_EQ(parsed.roundsFile, "rounds/rounds.json");
}

// 6. Asset Manager Placeholder Fallbacks
TEST_CASE(AssetManagerFallbacks) {
    auto& manager = btd4::AssetManager::instance();

    // When manifest not loaded or asset missing, fallback is reported
    TEST_ASSERT(manager.isUsingFallback("bloon_red"));
    TEST_ASSERT(manager.isUsingFallback("tower_dart_monkey"));

    // Check placeholder colors are valid
    auto redColor = manager.getPlaceholderColor("bloon_red");
    TEST_ASSERT_EQ(redColor.r, 255);
    TEST_ASSERT_EQ(redColor.g, 0);
    TEST_ASSERT_EQ(redColor.b, 0);

    auto blueColor = manager.getPlaceholderColor("bloon_blue");
    TEST_ASSERT(blueColor.b > 200);
}

// 7. Asset ID Resolvers
TEST_CASE(AssetManagerIdResolvers) {
    TEST_ASSERT_EQ(btd4::AssetManager::getBloonAssetId(btd4::BloonType::Red), "bloon_red");
    TEST_ASSERT_EQ(btd4::AssetManager::getBloonAssetId(btd4::BloonType::MOAB), "bloon_moab");
    TEST_ASSERT_EQ(btd4::AssetManager::getTowerAssetId(btd4::TowerType::DartMonkey), "tower_dart_monkey");
    TEST_ASSERT_EQ(btd4::AssetManager::getTowerAssetId(btd4::TowerType::SuperMonkey), "tower_super_monkey");
    TEST_ASSERT_EQ(btd4::AssetManager::getProjectileAssetId(btd4::ProjectileType::Dart), "projectile_dart");
    TEST_ASSERT_EQ(btd4::AssetManager::getProjectileAssetId(btd4::ProjectileType::Bomb), "projectile_bomb");
}

// 8. Placeholder Package and Entity Drawing
TEST_CASE(AssetManagerPlaceholderPackage) {
    auto& manager = btd4::AssetManager::instance();

    // Native file system to read placeholder assets
    btd4::NativeFileSystem fs;
    bool ok = manager.initialize(fs, "non_existent_dir_forces_placeholder");
    TEST_ASSERT(ok);
    TEST_ASSERT(manager.hasManifest());
    TEST_ASSERT_EQ(manager.manifest().packageName, "Bloons TD 4 Placeholder Data");
}

