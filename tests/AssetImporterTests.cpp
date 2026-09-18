#include "TestRunner.hpp"
#include "../tools/swf/SwfTypes.hpp"
#include "../tools/swf/SwfReader.hpp"
#include "../tools/swf/Inflate.hpp"
#include "../tools/swf/SwfParser.hpp"
#include "../tools/asset_converter/AssetConverter.hpp"
#include "../tools/asset_importer/AssetImporter.hpp"
#include "../tools/archive/ZipArchive.hpp"
#include "../engine/assets/AssetManifest.hpp"
#include "../engine/assets/AssetManager.hpp"
#include "../platform/common/NativeFileSystem.hpp"
#include "../builder/project/Project.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <iterator>

namespace {
void writeU16(std::ofstream& out, uint16_t value) { out.put(static_cast<char>(value & 0xFF)); out.put(static_cast<char>((value >> 8) & 0xFF)); }
void writeU32(std::ofstream& out, uint32_t value) { writeU16(out, static_cast<uint16_t>(value & 0xFFFF)); writeU16(out, static_cast<uint16_t>(value >> 16)); }
void appendStoredZipEntry(std::ofstream& out, const std::string& name, const std::string& data, uint32_t& localOffset) {
    localOffset = static_cast<uint32_t>(out.tellp()); writeU32(out, 0x04034b50); writeU16(out, 20); writeU16(out, 0); writeU16(out, 0); writeU16(out, 0); writeU16(out, 0); writeU32(out, 0); writeU32(out, static_cast<uint32_t>(data.size())); writeU32(out, static_cast<uint32_t>(data.size())); writeU16(out, static_cast<uint16_t>(name.size())); writeU16(out, 0); out.write(name.data(), static_cast<std::streamsize>(name.size())); out.write(data.data(), static_cast<std::streamsize>(data.size()));
}
void appendSwfTag(std::vector<uint8_t>& swf, uint16_t code, const std::vector<uint8_t>& payload) { const uint16_t header = static_cast<uint16_t>((code << 6) | payload.size()); swf.push_back(static_cast<uint8_t>(header & 0xFF)); swf.push_back(static_cast<uint8_t>((header >> 8) & 0xFF)); swf.insert(swf.end(), payload.begin(), payload.end()); }
}

TEST_CASE(SwfReaderBitstreamParsing) {
    std::vector<uint8_t> buffer = {0x28, 0x14, 0x05, 0x00}; btd4::swf::SwfReader reader(buffer); btd4::swf::SwfRect rect = reader.readRect();
    TEST_ASSERT_EQ(rect.xMinTwips, 0); TEST_ASSERT_EQ(rect.xMaxTwips, 10); TEST_ASSERT_EQ(rect.yMinTwips, 0); TEST_ASSERT_EQ(rect.yMaxTwips, 10);
}

TEST_CASE(InflateUncompressedBlock) {
    std::vector<uint8_t> deflateData = {0x01, 0x04, 0x00, 0xFB, 0xFF, 'T', 'E', 'S', 'T'}; std::vector<uint8_t> decompressed;
    bool ok = btd4::swf::Inflate::decompressDeflate(deflateData.data(), deflateData.size(), decompressed); TEST_ASSERT(ok); TEST_ASSERT_EQ(decompressed.size(), static_cast<size_t>(4)); TEST_ASSERT_EQ(std::string(decompressed.begin(), decompressed.end()), "TEST");
}

TEST_CASE(SwfParserSyntheticFWS) {
    std::vector<uint8_t> swf; swf.push_back('F'); swf.push_back('W'); swf.push_back('S'); swf.push_back(10); uint32_t fileLen = 34;
    swf.push_back(fileLen & 0xFF); swf.push_back((fileLen >> 8) & 0xFF); swf.push_back((fileLen >> 16) & 0xFF); swf.push_back((fileLen >> 24) & 0xFF);
    swf.push_back(0x28); swf.push_back(0x05); swf.push_back(0x00); swf.push_back(0x50); swf.push_back(0x00); swf.push_back(0x3C); swf.push_back(0x01); swf.push_back(0x00); swf.push_back(0x44); swf.push_back(0x11); swf.push_back(0x08); swf.push_back(0x00); swf.push_back(0x00); swf.push_back(0x00);
    std::string className = "MonkeyDart"; uint32_t symLength = 2 + 2 + className.size() + 1; uint16_t symTagHeader = (76 << 6) | (symLength & 0x3F);
    swf.push_back(symTagHeader & 0xFF); swf.push_back((symTagHeader >> 8) & 0xFF); swf.push_back(0x01); swf.push_back(0x00); swf.push_back(10); swf.push_back(0); for (char c : className) swf.push_back(static_cast<uint8_t>(c)); swf.push_back(0); swf.push_back(0x00); swf.push_back(0x00);
    btd4::swf::SwfParser parser; std::string err; bool ok = parser.parse(swf.data(), swf.size(), err); TEST_ASSERT(ok); TEST_ASSERT_EQ(parser.header().version, 10); TEST_ASSERT(parser.metadata().isActionScript3); TEST_ASSERT_EQ(parser.findSymbolName(10), "MonkeyDart"); TEST_ASSERT_EQ(parser.findCharacterId("MonkeyDart"), 10);
}

TEST_CASE(SwfParserResolvesBitmapBackedSymbols) {
    std::vector<uint8_t> swf = {'F','W','S',10,0,0,0,0,0x28,0x05,0x00,0x50,0x00,0x3C,0x01,0x00};
    appendSwfTag(swf, 21, {20, 0, 0xFF, 0xD8}); appendSwfTag(swf, 2, {10, 0, 0x00, 0x01, 0x40, 20, 0, 0x00, 0x00}); appendSwfTag(swf, 76, {1, 0, 10, 0, 'M','o','n','k','e','y','D','a','r','t',0}); appendSwfTag(swf, 0, {});
    const uint32_t fileLength = static_cast<uint32_t>(swf.size()); swf[4] = static_cast<uint8_t>(fileLength & 0xFF); swf[5] = static_cast<uint8_t>((fileLength >> 8) & 0xFF); swf[6] = static_cast<uint8_t>((fileLength >> 16) & 0xFF); swf[7] = static_cast<uint8_t>((fileLength >> 24) & 0xFF);
    btd4::swf::SwfParser parser; std::string err; TEST_ASSERT(parser.parse(swf.data(), swf.size(), err)); TEST_ASSERT_EQ(parser.images().size(), static_cast<size_t>(1)); TEST_ASSERT_EQ(parser.images()[0].characterId, static_cast<uint16_t>(20)); TEST_ASSERT_EQ(parser.images()[0].className, "MonkeyDart");
}

TEST_CASE(AssetImporterGeneratesRuntimeAliases) {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "btd4_asset_import_test"; const std::filesystem::path swfPath = root / "BTD4.swf"; const std::filesystem::path outputDir = root / "game_data"; std::error_code ec; std::filesystem::remove_all(root, ec); std::filesystem::create_directories(root, ec); TEST_ASSERT(!ec);
    std::vector<uint8_t> swf = {'F','W','S',10,0,0,0,0,0x28,0x05,0x00,0x50,0x00,0x3C,0x01,0x00}; appendSwfTag(swf, 21, {20, 0, 0xFF, 0xD8}); appendSwfTag(swf, 76, {1, 0, 20, 0, 'D','a','r','t','M','o','n','k','e','y',0}); appendSwfTag(swf, 0, {});
    const uint32_t fileLength = static_cast<uint32_t>(swf.size()); swf[4] = static_cast<uint8_t>(fileLength & 0xFF); swf[5] = static_cast<uint8_t>((fileLength >> 8) & 0xFF); swf[6] = static_cast<uint8_t>((fileLength >> 16) & 0xFF); swf[7] = static_cast<uint8_t>((fileLength >> 24) & 0xFF);
    { std::ofstream out(swfPath, std::ios::binary | std::ios::trunc); TEST_ASSERT(out.is_open()); out.write(reinterpret_cast<const char*>(swf.data()), static_cast<std::streamsize>(swf.size())); }
    btd4::tools::ImportOptions options; options.sourceSwf = swfPath.string(); options.outputDir = outputDir.string(); options.targetPlatform = "Windows"; btd4::tools::ImportReport report = btd4::tools::AssetImporter::run(options); TEST_ASSERT(report.success); TEST_ASSERT_EQ(report.texturesExtracted, static_cast<uint32_t>(1));
    std::ifstream manifest(outputDir / "manifest.json"); TEST_ASSERT(manifest.is_open()); std::string json((std::istreambuf_iterator<char>(manifest)), std::istreambuf_iterator<char>()); TEST_ASSERT(json.find("\"dart_monkey\": \"textures/dart_monkey.jpg\"") != std::string::npos); TEST_ASSERT(json.find("Native gameplay asset aliases") != std::string::npos);
    btd4::AssetManifest importedManifest;
    std::string manifestError;
    TEST_ASSERT(importedManifest.loadFromFile(btd4::NativeFileSystem{}, (outputDir / "manifest.json").string(), manifestError));
    TEST_ASSERT_EQ(importedManifest.source, swfPath.string());
    TEST_ASSERT(std::filesystem::is_regular_file(outputDir / "textures" / "dart_monkey.jpg")); std::filesystem::remove_all(root, ec);
}

TEST_CASE(AssetImporterCanRunTwiceOnSameOutput) {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "btd4_asset_import_repeat_test";
    const std::filesystem::path swfPath = root / "BTD4.swf";
    const std::filesystem::path outputDir = root / "game_data";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    TEST_ASSERT(!ec);

    std::vector<uint8_t> swf = {
        'F','W','S',10,0,0,0,0,
        0x28,0x05,0x00,0x50,0x00,0x3C,0x01,0x00
    };
    appendSwfTag(swf, 21, {20, 0, 0xFF, 0xD8});
    appendSwfTag(swf, 76, {1, 0, 20, 0, 'D','a','r','t','M','o','n','k','e','y',0});
    appendSwfTag(swf, 0, {});

    const uint32_t fileLength = static_cast<uint32_t>(swf.size());
    swf[4] = static_cast<uint8_t>(fileLength & 0xFF);
    swf[5] = static_cast<uint8_t>((fileLength >> 8) & 0xFF);
    swf[6] = static_cast<uint8_t>((fileLength >> 16) & 0xFF);
    swf[7] = static_cast<uint8_t>((fileLength >> 24) & 0xFF);

    {
        std::ofstream out(swfPath, std::ios::binary | std::ios::trunc);
        TEST_ASSERT(out.is_open());
        out.write(reinterpret_cast<const char*>(swf.data()), static_cast<std::streamsize>(swf.size()));
    }

    btd4::tools::ImportOptions options;
    options.sourceSwf = swfPath.string();
    options.outputDir = outputDir.string();
    options.targetPlatform = "Windows";

    btd4::tools::ImportReport first = btd4::tools::AssetImporter::run(options);
    TEST_ASSERT(first.success);

    btd4::tools::ImportReport second = btd4::tools::AssetImporter::run(options);
    TEST_ASSERT(second.success);
    TEST_ASSERT_EQ(second.texturesExtracted, static_cast<uint32_t>(1));
    TEST_ASSERT(std::filesystem::is_regular_file(outputDir / "manifest.json"));
    TEST_ASSERT(std::filesystem::is_regular_file(outputDir / "textures" / "dart_monkey.jpg"));

    std::filesystem::remove_all(root, ec);
}

TEST_CASE(AssetConverterNormalization) {
    TEST_ASSERT_EQ(btd4::tools::AssetConverter::normalizeIdentifier("MonkeyDart", "tex", 1), "monkey_dart"); TEST_ASSERT_EQ(btd4::tools::AssetConverter::normalizeIdentifier("TackTower_level2", "tex", 2), "tack_tower_level2"); TEST_ASSERT_EQ(btd4::tools::AssetConverter::normalizeIdentifier("", "fallback", 42), "fallback_42");
}

TEST_CASE(AssetManifestSerializationAndParsing) {
    btd4::AssetManifest manifest; manifest.packageName = "Test \"Package"; manifest.source = "C:\\Users\\Tester\\BTD4 \"Flash\".swf"; manifest.textures["dart\\monkey"] = "textures\\dart\\monkey.bmp"; manifest.audio["pop"] = "audio/pop.wav"; manifest.maps.push_back("maps/level1.json"); manifest.roundsFile = "rounds/rounds.json";
    std::string json = manifest.serialize(); TEST_ASSERT(json.find("\"package_name\": \"Test \\\"Package\"") != std::string::npos); TEST_ASSERT(json.find("\\Users\\\\Tester") != std::string::npos);
    btd4::AssetManifest parsed; std::string err; bool ok = parsed.parseJson(json, err); TEST_ASSERT(ok); TEST_ASSERT_EQ(parsed.packageName, "Test \"Package"); TEST_ASSERT_EQ(parsed.source, "C:\\Users\\Tester\\BTD4 \"Flash\".swf"); TEST_ASSERT_EQ(parsed.textures["dart\\monkey"], "textures\\dart\\monkey.bmp"); TEST_ASSERT_EQ(parsed.audio["pop"], "audio/pop.wav"); TEST_ASSERT_EQ(parsed.roundsFile, "rounds/rounds.json");
}

TEST_CASE(AssetManagerFallbacks) {
    auto& manager = btd4::AssetManager::instance(); TEST_ASSERT(manager.isUsingFallback("bloon_red")); TEST_ASSERT(manager.isUsingFallback("tower_dart_monkey")); auto redColor = manager.getPlaceholderColor("bloon_red"); TEST_ASSERT_EQ(redColor.r, 255); TEST_ASSERT_EQ(redColor.g, 0); TEST_ASSERT_EQ(redColor.b, 0); auto blueColor = manager.getPlaceholderColor("bloon_blue"); TEST_ASSERT(blueColor.b > 200);
}

TEST_CASE(AssetManagerIdResolvers) {
    TEST_ASSERT_EQ(btd4::AssetManager::getBloonAssetId(btd4::BloonType::Red), "bloon_red"); TEST_ASSERT_EQ(btd4::AssetManager::getBloonAssetId(btd4::BloonType::MOAB), "bloon_moab"); TEST_ASSERT_EQ(btd4::AssetManager::getTowerAssetId(btd4::TowerType::DartMonkey), "tower_dart_monkey"); TEST_ASSERT_EQ(btd4::AssetManager::getTowerAssetId(btd4::TowerType::SuperMonkey), "tower_super_monkey"); TEST_ASSERT_EQ(btd4::AssetManager::getProjectileAssetId(btd4::ProjectileType::Dart), "projectile_dart"); TEST_ASSERT_EQ(btd4::AssetManager::getProjectileAssetId(btd4::ProjectileType::Bomb), "projectile_bomb");
}

TEST_CASE(AssetManagerPlaceholderPackage) {
    auto& manager = btd4::AssetManager::instance(); btd4::NativeFileSystem fs; bool ok = manager.initialize(fs, "non_existent_dir_forces_placeholder"); TEST_ASSERT(ok); TEST_ASSERT(manager.hasManifest()); TEST_ASSERT_EQ(manager.manifest().packageName, "Bloons TD 4 Placeholder Data");
}

TEST_CASE(AssetImporterExtractsIpaResources) {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "btd4_asset_import_ipa_test";
    const std::filesystem::path swfPath = root / "BTD4.swf";
    const std::filesystem::path ipaPath = root / "BTD4HD.ipa";
    const std::filesystem::path outputDir = root / "game_data";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    TEST_ASSERT(!ec);

    std::vector<uint8_t> swf = {'F','W','S',10,0,0,0,0,0x28,0x05,0x00,0x50,0x00,0x3C,0x01,0x00};
    appendSwfTag(swf, 21, {20, 0, 0xFF, 0xD8});
    appendSwfTag(swf, 76, {1, 0, 20, 0, 'D','a','r','t','M','o','n','k','e','y',0});
    appendSwfTag(swf, 0, {});
    const uint32_t fileLength = static_cast<uint32_t>(swf.size());
    swf[4] = static_cast<uint8_t>(fileLength & 0xFF);
    swf[5] = static_cast<uint8_t>((fileLength >> 8) & 0xFF);
    swf[6] = static_cast<uint8_t>((fileLength >> 16) & 0xFF);
    swf[7] = static_cast<uint8_t>((fileLength >> 24) & 0xFF);
    {
        std::ofstream out(swfPath, std::ios::binary | std::ios::trunc);
        TEST_ASSERT(out.is_open());
        out.write(reinterpret_cast<const char*>(swf.data()), static_cast<std::streamsize>(swf.size()));
    }

    const std::string entryName = "Payload/BTD4HD.app/Assets/hd_map.dat";
    const std::string entryData = "HD-ASSET";
    {
        std::ofstream out(ipaPath, std::ios::binary | std::ios::trunc);
        TEST_ASSERT(out.is_open());
        uint32_t localOffset = 0;
        appendStoredZipEntry(out, entryName, entryData, localOffset);
        const uint32_t directoryOffset = static_cast<uint32_t>(out.tellp());
        writeU32(out, 0x02014b50);
        writeU16(out, 20); writeU16(out, 20); writeU16(out, 0); writeU16(out, 0);
        writeU16(out, 0); writeU16(out, 0); writeU32(out, 0);
        writeU32(out, static_cast<uint32_t>(entryData.size()));
        writeU32(out, static_cast<uint32_t>(entryData.size()));
        writeU16(out, static_cast<uint16_t>(entryName.size())); writeU16(out, 0); writeU16(out, 0);
        writeU16(out, 0); writeU16(out, 0); writeU32(out, 0); writeU32(out, localOffset);
        out.write(entryName.data(), static_cast<std::streamsize>(entryName.size()));
        const uint32_t directorySize = static_cast<uint32_t>(out.tellp()) - directoryOffset;
        writeU32(out, 0x06054b50); writeU16(out, 0); writeU16(out, 0); writeU16(out, 1);
        writeU16(out, 1); writeU32(out, directorySize); writeU32(out, directoryOffset); writeU16(out, 0);
    }

    btd4::tools::ImportOptions options;
    options.sourceSwf = swfPath.string();
    options.sourceIpa = ipaPath.string();
    options.outputDir = outputDir.string();
    options.targetPlatform = "Windows";
    const btd4::tools::ImportReport report = btd4::tools::AssetImporter::run(options);
    TEST_ASSERT(report.success);
    TEST_ASSERT(report.ipaDetected);
    TEST_ASSERT(report.ipaArchiveDetected);
    TEST_ASSERT_EQ(report.ipaFilesExtracted, static_cast<uint32_t>(1));
    TEST_ASSERT_EQ(report.ipaBytesExtracted, static_cast<uint64_t>(entryData.size()));
    TEST_ASSERT_EQ(report.ipaOutputDirectory, "mobile");
    TEST_ASSERT(std::filesystem::is_regular_file(outputDir / "mobile" / "phone" / entryName));
    TEST_ASSERT(std::ifstream(outputDir / "mobile" / entryName).good());

    std::ifstream manifest(outputDir / "manifest.json");
    TEST_ASSERT(manifest.is_open());
    std::string json((std::istreambuf_iterator<char>(manifest)), std::istreambuf_iterator<char>());
    TEST_ASSERT(json.find("\"ipa_files_extracted\": 1") != std::string::npos);
    TEST_ASSERT(json.find("\"ipa_output_directory\": \"mobile\"") != std::string::npos);

    std::filesystem::remove_all(root, ec);
}

TEST_CASE(ZipArchiveStoredEntry) {
    const std::filesystem::path testPath = std::filesystem::temp_directory_path() / "btd4_test_ipa.zip"; const std::string name = "Payload/Test.app/Info.plist"; const std::string payload = "plist-test-data";
    { std::ofstream out(testPath, std::ios::binary | std::ios::trunc); TEST_ASSERT(out.is_open()); uint32_t localOffset = 0; appendStoredZipEntry(out, name, payload, localOffset); const uint32_t directoryOffset = static_cast<uint32_t>(out.tellp()); writeU32(out, 0x02014b50); writeU16(out, 20); writeU16(out, 20); writeU16(out, 0); writeU16(out, 0); writeU16(out, 0); writeU16(out, 0); writeU32(out, 0); writeU32(out, static_cast<uint32_t>(payload.size())); writeU32(out, static_cast<uint32_t>(payload.size())); writeU16(out, static_cast<uint16_t>(name.size())); writeU16(out, 0); writeU16(out, 0); writeU16(out, 0); writeU16(out, 0); writeU32(out, 0); writeU32(out, localOffset); out.write(name.data(), static_cast<std::streamsize>(name.size())); const uint32_t directorySize = static_cast<uint32_t>(out.tellp()) - directoryOffset; writeU32(out, 0x06054b50); writeU16(out, 0); writeU16(out, 0); writeU16(out, 1); writeU16(out, 1); writeU32(out, directorySize); writeU32(out, directoryOffset); writeU16(out, 0); }
    btd4::tools::ZipArchive archive; std::string error; TEST_ASSERT(archive.open(testPath.string(), error)); TEST_ASSERT_EQ(archive.entries().size(), static_cast<size_t>(1)); TEST_ASSERT(archive.contains(name)); std::vector<uint8_t> result; TEST_ASSERT(archive.readEntry(name, result, error)); TEST_ASSERT_EQ(std::string(result.begin(), result.end()), payload); std::error_code ec; std::filesystem::remove(testPath, ec);
}

TEST_CASE(ProjectDiscoversDroppedAssets) {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "btd4_builder_assets"; std::error_code ec; std::filesystem::remove_all(root, ec); std::filesystem::create_directories(root / "nested", ec); TEST_ASSERT(!ec);
    std::ofstream swf(root / "nested" / "BTD4.swf", std::ios::binary); swf << "synthetic test asset"; swf.close(); std::ofstream ipa(root / "MobileContent.IPA", std::ios::binary); ipa << "synthetic test package"; ipa.close();
    btd4::Project project; TEST_ASSERT(project.discoverSourceAssets(root.string())); TEST_ASSERT(project.hasValidSwf()); TEST_ASSERT(project.hasValidIpa()); TEST_ASSERT(project.config().sourceSwf.find("BTD4.swf") != std::string::npos); TEST_ASSERT(project.config().sourceIpa.find("MobileContent.IPA") != std::string::npos); std::filesystem::remove_all(root, ec);
}


TEST_CASE(ProjectClassifiesMultipleSourceFiles) {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "btd4_project_multi_source_test";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    TEST_ASSERT(!ec);

    {
        std::ofstream(root / "Expansion.swf") << "expansion";
        std::ofstream(root / "BTD4.swf") << "base";
        std::ofstream(root / "BTD4 HD iPad.ipa") << "hd";
        std::ofstream(root / "BTD4 Mobile.ipa") << "mobile";
    }

    btd4::Project project;
    TEST_ASSERT(project.discoverSourceAssets(root.string()));
    TEST_ASSERT(project.config().sourceSwf.find("BTD4.swf") != std::string::npos);
    TEST_ASSERT(project.config().sourceExpansionSwf.find("Expansion.swf") != std::string::npos);
    TEST_ASSERT(project.config().sourceHdIpa.find("BTD4 HD iPad.ipa") != std::string::npos);
    TEST_ASSERT(project.config().sourceMobileIpa.find("BTD4 Mobile.ipa") != std::string::npos);

    std::filesystem::remove_all(root, ec);
}

TEST_CASE(ProjectSerializeEscapesConfiguredPaths) {
    btd4::Project project;
    project.config().projectName = "BTD4 \"Definitive\"";
    project.config().sourceDirectory = R"(C:\Games\BTD4\Sources)";
    project.config().sourceSwf = R"(C:\Games\BTD4\My "Base".swf)";
    project.config().sourceIpa = R"(C:\Games\BTD4\Phone.ipa)";
    project.config().sourceExpansionSwf = R"(C:\Games\BTD4\Expansion.swf)";
    project.config().sourceHdIpa = R"(C:\Games\BTD4\HD iPad.ipa)";
    project.config().sourceMobileIpa = R"(C:\Games\BTD4\Mobile.ipa)";
    project.config().gameEdition = "Definitive Edition";
    project.config().targetPlatform = "Windows";

    const std::string json = project.serialize();
    TEST_ASSERT(json.find(R"("project_name": "BTD4 \"Definitive\"")") != std::string::npos);
    TEST_ASSERT(json.find(R"("source_swf": "C:\\Games\\BTD4\\My \"Base\".swf")") != std::string::npos);

    btd4::Project roundTrip;
    TEST_ASSERT(roundTrip.deserialize(json));
    TEST_ASSERT_EQ(roundTrip.config().projectName, "BTD4 \"Definitive\"");
    TEST_ASSERT_EQ(roundTrip.config().sourceSwf, R"(C:\Games\BTD4\My "Base".swf)");
    TEST_ASSERT_EQ(roundTrip.config().sourceExpansionSwf, R"(C:\Games\BTD4\Expansion.swf)");
    TEST_ASSERT_EQ(roundTrip.config().sourceHdIpa, R"(C:\Games\BTD4\HD iPad.ipa)");
    TEST_ASSERT_EQ(roundTrip.config().sourceMobileIpa, R"(C:\Games\BTD4\Mobile.ipa)");
}

TEST_CASE(ProjectRescanInvalidDirectoryPreservesSources) {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "btd4_project_rescan_test";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    TEST_ASSERT(!ec);

    std::ofstream(root / "BTD4.swf") << "base";
    btd4::Project project;
    TEST_ASSERT(project.discoverSourceAssets(root.string()));
    const std::string original = project.config().sourceSwf;

    TEST_ASSERT(!project.discoverSourceAssets((root / "does-not-exist").string()));
    TEST_ASSERT_EQ(project.config().sourceSwf, original);

    std::filesystem::remove_all(root, ec);
}

TEST_CASE(AssetImporterSeparatesPhoneAndMobileIpaLayers) {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "btd4_asset_import_ipa_layers_test";
    const std::filesystem::path swfPath = root / "BTD4.swf";
    const std::filesystem::path phoneIpa = root / "Phone.ipa";
    const std::filesystem::path mobileIpa = root / "Mobile.ipa";
    const std::filesystem::path outputDir = root / "game_data";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    TEST_ASSERT(!ec);

    std::vector<uint8_t> swf = {'F','W','S',10,0,0,0,0,0x28,0x05,0x00,0x50,0x00,0x3C,0x01,0x00};
    appendSwfTag(swf, 21, {20, 0, 0xFF, 0xD8});
    appendSwfTag(swf, 76, {1, 0, 20, 0, 'D','a','r','t','M','o','n','k','e','y',0});
    appendSwfTag(swf, 0, {});
    const uint32_t fileLength = static_cast<uint32_t>(swf.size());
    swf[4] = static_cast<uint8_t>(fileLength & 0xFF);
    swf[5] = static_cast<uint8_t>((fileLength >> 8) & 0xFF);
    swf[6] = static_cast<uint8_t>((fileLength >> 16) & 0xFF);
    swf[7] = static_cast<uint8_t>((fileLength >> 24) & 0xFF);
    {
        std::ofstream out(swfPath, std::ios::binary | std::ios::trunc);
        TEST_ASSERT(out.is_open());
        out.write(reinterpret_cast<const char*>(swf.data()), static_cast<std::streamsize>(swf.size()));
    }

    const auto writeIpa = [](const std::filesystem::path& ipa, const std::string& filename, const std::string& bytes) {
        std::ofstream out(ipa, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) return false;
        uint32_t localOffset = 0;
        const std::string entryName = "Payload/Test.app/" + filename;
        appendStoredZipEntry(out, entryName, bytes, localOffset);
        const uint32_t directoryOffset = static_cast<uint32_t>(out.tellp());
        writeU32(out, 0x02014b50);
        writeU16(out, 20); writeU16(out, 20); writeU16(out, 0); writeU16(out, 0);
        writeU16(out, 0); writeU16(out, 0); writeU32(out, 0);
        writeU32(out, static_cast<uint32_t>(bytes.size()));
        writeU32(out, static_cast<uint32_t>(bytes.size()));
        writeU16(out, static_cast<uint16_t>(entryName.size())); writeU16(out, 0); writeU16(out, 0);
        writeU16(out, 0); writeU16(out, 0); writeU32(out, 0); writeU32(out, localOffset);
        out.write(entryName.data(), static_cast<std::streamsize>(entryName.size()));
        const uint32_t directorySize = static_cast<uint32_t>(out.tellp()) - directoryOffset;
        writeU32(out, 0x06054b50); writeU16(out, 0); writeU16(out, 0); writeU16(out, 1);
        writeU16(out, 1); writeU32(out, directorySize); writeU32(out, directoryOffset); writeU16(out, 0);
        return out.good();
    };

    TEST_ASSERT(writeIpa(phoneIpa, "phone_marker.dat", "PHONE"));
    TEST_ASSERT(writeIpa(mobileIpa, "mobile_marker.dat", "MOBILE"));

    btd4::tools::ImportOptions options;
    options.sourceSwf = swfPath.string();
    options.sourceIpa = phoneIpa.string();
    options.sourceMobileIpa = mobileIpa.string();
    options.outputDir = outputDir.string();
    options.targetPlatform = "Windows";

    const btd4::tools::ImportReport report = btd4::tools::AssetImporter::run(options);
    TEST_ASSERT(report.success);
    TEST_ASSERT(std::filesystem::is_regular_file(outputDir / "mobile" / "phone" / "Payload" / "Test.app" / "phone_marker.dat"));
    TEST_ASSERT(std::filesystem::is_regular_file(outputDir / "mobile" / "mobile" / "Payload" / "Test.app" / "mobile_marker.dat"));

    std::filesystem::remove_all(root, ec);
}

TEST_CASE(InflateDynamicHuffmanStream) {
    const std::vector<uint8_t> zlibData = {
        0x78,0xDA,0xCB,0x48,0xCD,0xC9,0xC9,0x57,0xC8,0x40,0x27,0x01,0x68,0x03,0x08,0xB1
    };
    std::vector<uint8_t> output;
    TEST_ASSERT(btd4::swf::Inflate::decompressZlib(zlibData.data(), zlibData.size(), output, 23));
    TEST_ASSERT_EQ(std::string(output.begin(), output.end()), "hello hello hello hello");
}

TEST_CASE(SwfParserParsesCompressedHeader) {
    const std::vector<uint8_t> swf = {
        'C','W','S',10,0x12,0x00,0x00,0x00,
        0x78,0xDA,0xD3,0x60,0x65,0x08,0x60,0xB0,0x61,0x64,0x60,0x60,0x00,0x00,0x05,0x27,0x00,0xBB
    };
    btd4::swf::SwfParser parser;
    std::string error;
    TEST_ASSERT(parser.parse(swf.data(), swf.size(), error));
    TEST_ASSERT_EQ(parser.header().version, static_cast<uint8_t>(10));
}
