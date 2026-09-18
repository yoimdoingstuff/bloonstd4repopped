#include "AssetImporter.hpp"
#include "../swf/SwfParser.hpp"
#include "../asset_converter/AssetConverter.hpp"
#include "../archive/ZipArchive.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <iostream>
#include <unordered_set>

namespace btd4::tools {
namespace fs = std::filesystem;
namespace {
std::string lower(std::string value){std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return value;}
bool containsAny(const std::string& value,const std::initializer_list<const char*>& needles){const std::string n=lower(value);for(const char* needle:needles)if(n.find(needle)!=std::string::npos)return true;return false;}
bool looksLikeIpaArchive(const std::string& path){std::ifstream file(path,std::ios::binary);if(!file)return false;unsigned char s[4]{};file.read(reinterpret_cast<char*>(s),4);return file.gcount()==4&&s[0]=='P'&&s[1]=='K'&&(s[2]==3||s[2]==5||s[2]==7)&&(s[3]==4||s[3]==6||s[3]==8);}
void addFeature(ImportReport& report,const std::string& feature){if(std::find(report.detectedFeatures.begin(),report.detectedFeatures.end(),feature)==report.detectedFeatures.end())report.detectedFeatures.push_back(feature);}

std::string nativeAssetAlias(const std::string& name){
    const std::string n=lower(name);
    if(containsAny(n,{"dartmonkey","dart_monkey","dart monkey","dartmonk"}))return "tower_dart_monkey";
    if(containsAny(n,{"tackshooter","tack_shooter","tack shooter","tacktower"}))return "tower_tack_shooter";
    if(containsAny(n,{"boomerangthrower","boomerang_thrower","boomerang thrower","boomerangmonkey"}))return "tower_boomerang";
    if(containsAny(n,{"bombtower","bomb_tower","bomb tower","bombshooter"}))return "tower_bomb_tower";
    if(containsAny(n,{"icetower","ice_tower","ice tower","icemonkey","ice monkey"}))return "tower_ice";
    if(containsAny(n,{"gluegunner","glue_gunner","glue gunner","monkeyglue","monkey glue"}))return "tower_glue_gunner";
    if(containsAny(n,{"monkeybeacon","monkey_beacon","monkey beacon"}))return "tower_monkey_beacon";
    if(containsAny(n,{"monkeyace","monkey_ace","monkey ace"}))return "tower_monkey_ace";
    if(containsAny(n,{"monkeybuccaneer","monkey_buccaneer","monkey buccaneer","buccaneer"}))return "tower_buccaneer";
    if(containsAny(n,{"wizardmonkey","wizard_monkey","wizard monkey"}))return "tower_wizard";
    if(containsAny(n,{"bananafarm","banana_farm","banana farm"}))return "tower_banana_farm";
    if(containsAny(n,{"mortarmonkey","mortar_monkey","mortar monkey","mortartower"}))return "tower_mortar";
    if(containsAny(n,{"dartlinggunner","dartling_gunner","dartling gunner"}))return "tower_dartling_gunner";
    if(containsAny(n,{"spikefactory","spike_factory","spike factory"}))return "tower_spike_factory";
    if(containsAny(n,{"beekeeper","bee_keeper","bee keeper"}))return "tower_beekeeper";
    if(containsAny(n,{"supermonkey","super_monkey","super monkey","supertower"}))return "tower_super_monkey";
    if(containsAny(n,{"dartprojectile","dart_projectile","dart projectile","dartshot","dart_shot","dart shot"}))return "projectile_dart";
    if(containsAny(n,{"tackprojectile","tack_projectile","tack projectile","tackshot","tack_shot","tack shot"}))return "projectile_tack";
    if(containsAny(n,{"bombprojectile","bomb_projectile","bomb projectile","bombshot","bomb_shot","bomb shot"}))return "projectile_bomb";
    if(containsAny(n,{"boomerangprojectile","boomerang_projectile","boomerang projectile","boomerangshot","boomerang_shot","boomerang shot"}))return "projectile_boomerang";
    if(containsAny(n,{"snipershot","sniper_shot","sniper shot","bulletprojectile","bullet_projectile","bullet projectile"}))return "projectile_bullet";
    if(containsAny(n,{"laserprojectile","laser_projectile","laser projectile","laserbeam","laser_beam","laser beam"}))return "projectile_laser";
    if(containsAny(n,{"plasmashot","plasma_shot","plasma shot","plasmaprojectile","plasma_projectile","plasma projectile"}))return "projectile_plasma";
    if(containsAny(n,{"redbloon","red_bloon","red bloon"}))return "bloon_red";
    if(containsAny(n,{"bluebloon","blue_bloon","blue bloon"}))return "bloon_blue";
    if(containsAny(n,{"greenbloon","green_bloon","green bloon"}))return "bloon_green";
    if(containsAny(n,{"yellowbloon","yellow_bloon","yellow bloon"}))return "bloon_yellow";
    if(containsAny(n,{"pinkbloon","pink_bloon","pink bloon"}))return "bloon_pink";
    if(containsAny(n,{"blackbloon","black_bloon","black bloon"}))return "bloon_black";
    if(containsAny(n,{"whitebloon","white_bloon","white bloon"}))return "bloon_white";
    if(containsAny(n,{"leadbloon","lead_bloon","lead bloon"}))return "bloon_lead";
    if(containsAny(n,{"rainbowbloon","rainbow_bloon","rainbow bloon"}))return "bloon_rainbow";
    if(containsAny(n,{"ceramicbloon","ceramic_bloon","ceramic bloon"}))return "bloon_ceramic";
    if(containsAny(n,{"moabbloon","moab_bloon","moab bloon","moab"}))return "bloon_moab";
    if(containsAny(n,{"mapbackground","map_background","map background","levelbackground","level_background","level background","trackbackground","track_background","track background"}))return "map_background";
    return {};
}

std::string detectLayerName(const std::string& path, const std::string& fallback) {
    const std::string n = lower(path);
    if (n.find("expansion") != std::string::npos) return "expansion";
    if (n.find("hd") != std::string::npos || n.find("ipad") != std::string::npos) return "hd";
    if (n.find("mobile") != std::string::npos || n.find("iphone") != std::string::npos || n.find("phone") != std::string::npos) return "mobile";
    if (n.find("flash") != std::string::npos || n.find("swf") != std::string::npos) return "flash";
    return fallback;
}

std::string escapeJsonString(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (unsigned char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    const char* hex = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(c >> 4) & 0xF];
                    out += hex[c & 0xF];
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

void addManifestEntry(std::vector<std::string>& entries,
                      std::unordered_set<std::string>& ids,
                      const std::string& id,
                      const std::string& path) {
    if (id.empty() || !ids.insert(id).second) return;
    entries.push_back("    \"" + escapeJsonString(id) +
                      "\": \"" + escapeJsonString(path) + "\"");
}

void inspectIpa(const std::string& path,
                const fs::path& outputDir,
                const std::string& layerName,
                ImportReport& report,
                const std::function<void(const std::string&)>& emitLog){
    if(path.empty()||!fs::exists(path))return;
    report.ipaDetected=true;
    report.ipaArchiveDetected=looksLikeIpaArchive(path);
    if(!report.ipaArchiveDetected){
        report.warnings.push_back("IPA path exists but does not have a standard ZIP/IPA signature.");
        emitLog("[Importer] IPA supplied, but archive signature was not recognized.");
        return;
    }

    try {
        emitLog("[Importer] Opening IPA archive for indexing and resource extraction...");
        ZipArchive archive;
        std::string archiveError;
        if(!archive.open(path,archiveError)){
            report.warnings.push_back("IPA archive could not be indexed: "+archiveError);
            emitLog("[Importer Warning] "+archiveError);
            return;
        }

        size_t payloadBundles=0,plistFiles=0;
        for(const auto& entry:archive.entries()){
            const std::string n=lower(entry.name);
            if(n.rfind("payload/",0)==0)addFeature(report,"Payload bundle");
            if(n.size()>=9&&n.rfind("info.plist")==n.size()-9){++plistFiles;addFeature(report,"Info.plist");}
            if(n.find(".app/")!=std::string::npos&&n.find("/documents/")!=std::string::npos)addFeature(report,"App document resources");
            if(n.find("beekeeper")!=std::string::npos)addFeature(report,"Beekeeper candidate");
            if(n.find("bloon")!=std::string::npos||n.find("tower")!=std::string::npos)addFeature(report,"BTD gameplay resource candidates");
            if(n.rfind("payload/",0)==0&&n.find(".app/")!=std::string::npos)++payloadBundles;
        }

        emitLog("[Importer] IPA index parsed: "+std::to_string(archive.entries().size())+
                " entries, "+std::to_string(payloadBundles)+" app resources, "+
                std::to_string(plistFiles)+" plist entries.");

        const fs::path mobileRoot=outputDir/"mobile"/layerName;
        std::error_code dirEc;
        fs::create_directories(mobileRoot,dirEc);
        if(dirEc){
            report.warnings.push_back("Could not create IPA resource directory: "+dirEc.message());
            emitLog("[Importer Warning] Could not create IPA resource directory: "+dirEc.message());
        }else{
            report.ipaOutputDirectory="mobile";
            uint32_t progressCounter=0;
            for(const auto& entry:archive.entries()){
                if(entry.name.empty() || entry.name.back()=='/') continue;
                std::string normalized=entry.name;
                std::replace(normalized.begin(),normalized.end(),'\\','/');
                while(!normalized.empty() && normalized.front()=='/') normalized.erase(normalized.begin());
                if(normalized.empty() || lower(normalized).rfind("payload/",0)!=0) continue;

                fs::path relative(normalized);
                if(relative.is_absolute()) continue;
                bool unsafe=false;
                for(const auto& part:relative){
                    const std::string component=part.string();
                    if(component.empty() || component=="." || component==".." ||
                       component.find(':')!=std::string::npos){
                        unsafe=true;
                        break;
                    }
                }
                if(unsafe){
                    report.warnings.push_back("Skipped unsafe IPA path: "+entry.name);
                    continue;
                }

                std::vector<uint8_t> data;
                if(!archive.readEntry(entry.name,data,archiveError)){
                    report.warnings.push_back("Could not extract IPA resource "+entry.name+": "+archiveError);
                    continue;
                }

                const fs::path target=mobileRoot/relative;
                std::error_code targetEc;
                fs::create_directories(target.parent_path(),targetEc);
                if(targetEc){
                    report.warnings.push_back("Could not create IPA resource directory for "+entry.name+": "+targetEc.message());
                    continue;
                }

                std::ofstream out(target,std::ios::binary|std::ios::trunc);
                if(!out.is_open()){
                    report.warnings.push_back("Could not write IPA resource: "+target.string());
                    continue;
                }
                if(!data.empty()) out.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(data.size()));
                if(!out.good()){
                    report.warnings.push_back("Could not finish writing IPA resource: "+target.string());
                    continue;
                }

                ++report.ipaFilesExtracted;
                report.ipaBytesExtracted+=static_cast<uint64_t>(data.size());
                if(++progressCounter%25==0){
                    emitLog("[Importer] Extracted "+std::to_string(report.ipaFilesExtracted)+" IPA resources...");
                }
            }

            emitLog("[Importer] Extracted "+std::to_string(report.ipaFilesExtracted)+
                    " IPA resources ("+std::to_string(report.ipaBytesExtracted)+
                    " bytes) to "+(outputDir/"mobile").string()+".");
            addFeature(report,"Extracted mobile resources");
        }

        if(!archive.contains("Info.plist")){
            for(const auto& entry:archive.entries()){
                const std::string n=lower(entry.name);
                if(n.rfind("payload/",0)==0&&n.size()>=9&&n.rfind("info.plist")==n.size()-9){
                    std::vector<uint8_t> data;
                    if(archive.readEntry(entry.name,data,archiveError)){
                        addFeature(report,"Readable app Info.plist");
                        emitLog("[Importer] Read app Info.plist: "+entry.name+" ("+std::to_string(data.size())+" bytes).");
                    }else{
                        report.warnings.push_back("Could not read app Info.plist: "+archiveError);
                    }
                    break;
                }
            }
        }
    } catch(const std::bad_alloc&) {
        report.warnings.push_back("IPA extraction ran out of memory; continuing with SWF-only import.");
        emitLog("[Importer Warning] IPA extraction ran out of memory; continuing with SWF-only import.");
    } catch(const std::exception& e) {
        report.warnings.push_back(std::string("IPA indexing/extraction failed: ")+e.what());
        emitLog(std::string("[Importer Warning] IPA indexing/extraction failed: ")+e.what());
    } catch(...) {
        report.warnings.push_back("IPA indexing/extraction failed with an unknown error.");
        emitLog("[Importer Warning] IPA indexing/extraction failed with an unknown error.");
    }
}
std::string detectSourceFamily(const swf::SwfParser& parser,bool& isBtd4,std::vector<std::string>& features){int score=0,gameplaySymbols=0;for(const auto& symbol:parser.symbols()){const std::string name=lower(symbol.second);if(containsAny(name,{"bloon","monkey","tower","dart","tack","boomerang","sniper","bomb"})){++gameplaySymbols;++score;}if(name.find("bloon")!=std::string::npos)features.push_back("Bloon symbols");if(name.find("tower")!=std::string::npos||name.find("monkey")!=std::string::npos)features.push_back("Tower symbols");if(name.find("map")!=std::string::npos)features.push_back("Map symbols");}for(const auto& image:parser.images())if(containsAny(lower(image.className),{"bloon","monkey","tower","dart","tack","boomerang","sniper","bomb"}))++score;std::sort(features.begin(),features.end());features.erase(std::unique(features.begin(),features.end()),features.end());isBtd4=gameplaySymbols>=3&&score>=5;return isBtd4?"BloonsTD4Flash":"UnknownSWF";}
}

ImportReport AssetImporter::run(const ImportOptions& options,LogCallback logCallback,ProgressCallback progressCallback){ImportReport report;report.sourceFile=options.sourceSwf;report.targetPlatform=options.targetPlatform;auto emitLog=[&](const std::string& msg){report.logMessages.push_back(msg);if(logCallback)logCallback(msg);};emitLog("[Importer] Initializing BTD4 Asset Import Pipeline...");emitLog("[Importer] Target platform: "+options.targetPlatform);if(options.sourceSwf.empty()){report.errorMessage="No source SWF file specified";emitLog("[Importer Error] "+report.errorMessage);return report;}if(!fs::exists(options.sourceSwf)){report.errorMessage="Source file does not exist: "+options.sourceSwf;emitLog("[Importer Error] "+report.errorMessage);return report;}emitLog("[Importer] Inspecting source: "+options.sourceSwf);swf::SwfParser parser;std::string parseError;auto swfProgress=[&](float p,const std::string& status){if(progressCallback)progressCallback(p*.5f,status);};if(!parser.parseFile(options.sourceSwf,parseError,swfProgress)){report.errorMessage="SWF parsing failed: "+parseError;emitLog("[Importer Error] "+report.errorMessage);return report;}report.swfVersion=parser.header().version;report.symbolsMapped=(uint32_t)parser.symbols().size();report.warnings=parser.warnings();emitLog("[Importer] SWF Header parsed successfully (Version: "+std::to_string(report.swfVersion)+", Size: "+std::to_string(parser.header().frameSize.widthPixels())+"x"+std::to_string(parser.header().frameSize.heightPixels())+");");emitLog("[Importer] Found "+std::to_string(parser.images().size())+" images, "+std::to_string(parser.sounds().size())+" audio streams, "+std::to_string(report.symbolsMapped)+" exported symbols.");report.sourceFamily=detectSourceFamily(parser,report.btd4Detected,report.detectedFeatures);if(report.btd4Detected)emitLog("[Importer] BTD4-like Flash content detected from exported gameplay symbols/assets.");else emitLog("[Importer] No confident BTD4 Flash signature was detected; continuing in generic SWF mode.");if(options.targetPlatform=="PSP"){addFeature(report,"PSP asset profile");emitLog("[Importer] PSP profile enabled: preserving source textures while keeping the runtime at 480x272 logical coordinates.");}else if(options.targetPlatform=="Windows"||options.targetPlatform=="Linux"){addFeature(report,"Desktop asset profile");emitLog("[Importer] Desktop profile enabled: using source-resolution assets with logical-resolution scaling in the runtime.");}else if(options.targetPlatform=="Xbox 360"){addFeature(report,"Xbox 360 asset profile");emitLog("[Importer] Xbox 360 profile selected; platform packaging remains dependent on the available backend/toolchain.");}std::sort(report.detectedFeatures.begin(),report.detectedFeatures.end());report.detectedFeatures.erase(std::unique(report.detectedFeatures.begin(),report.detectedFeatures.end()),report.detectedFeatures.end());fs::path outDir=options.outputDir,texturesDir=outDir/"textures",audioDir=outDir/"audio",mapsDir=outDir/"maps",roundsDir=outDir/"rounds";try{fs::create_directories(texturesDir);fs::create_directories(audioDir);fs::create_directories(mapsDir);fs::create_directories(roundsDir);}catch(const std::exception& e){report.errorMessage=std::string("Failed to create output directory: ")+e.what();emitLog("[Importer Error] "+report.errorMessage);return report;}inspectIpa(options.sourceIpa,outDir,"phone",report,emitLog);
    inspectIpa(options.sourceMobileIpa,outDir,"phone",report,emitLog);
    inspectIpa(options.sourceHdIpa,outDir,"hd",report,emitLog);std::vector<std::string> textureManifestEntries;std::unordered_set<std::string> textureIds;
    auto importAdditionalSwfLayer = [&](const std::string& sourcePath, const std::string& layerName) {
        if (sourcePath.empty() || !fs::is_regular_file(sourcePath)) return;
        swf::SwfParser layerParser;
        std::string layerError;
        if (!layerParser.parseFile(sourcePath, layerError, nullptr)) {
            report.warnings.push_back("Could not parse " + layerName + " SWF: " + layerError);
            return;
        }
        const fs::path layerDir = texturesDir / "layers" / layerName;
        std::error_code layerEc;
        fs::create_directories(layerDir, layerEc);
        if (layerEc) {
            report.warnings.push_back("Could not create " + layerName + " asset directory: " + layerEc.message());
            return;
        }
        ++report.sourceLayersImported;
        addFeature(report, layerName + " source layer");
        for (const auto& img : layerParser.images()) {
            const std::string baseId = AssetConverter::normalizeIdentifier(img.className, "tex", img.characterId);
            const std::string ext = img.format == swf::ImageFormat::JPEG ? ".jpg" : ".bmp";
            const std::string fileName = baseId + ext;
            const fs::path filePath = layerDir / fileName;
            if (!AssetConverter::saveImage(img, filePath.string())) continue;
            const std::string relative = "textures/layers/" + layerName + "/" + fileName;
            addManifestEntry(textureManifestEntries, textureIds, baseId + "@" + layerName, relative);
            if (layerName == "expansion" && textureIds.find(baseId) == textureIds.end())
                addManifestEntry(textureManifestEntries, textureIds, baseId, relative);
        }
        emitLog("[Importer] Imported " + layerName + " SWF layer: " + std::to_string(layerParser.images().size()) + " images.");
    };
    importAdditionalSwfLayer(options.sourceExpansionSwf, "expansion");
    importAdditionalSwfLayer(options.sourceSwf, "flash");

    auto indexIpaImages = [&](const std::string& layer) {
        const fs::path root = outDir / "mobile" / layer / "Payload";
        std::error_code ec;
        if (!fs::is_directory(root, ec)) return;
        for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
             it != end && !ec; it.increment(ec)) {
            if (!it->is_regular_file(ec)) continue;
            const std::string ext = lower(it->path().extension().string());
            if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".bmp") continue;
            const std::string baseId = btd4::tools::AssetConverter::normalizeIdentifier(it->path().stem().string(), "mobile_tex", 0);
            const std::string relative = (fs::path("mobile") / layer / fs::relative(it->path(), outDir / "mobile" / layer)).generic_string();
            addManifestEntry(textureManifestEntries, textureIds, baseId + "@" + layer, relative);
        }
    };

    indexIpaImages("phone");
    indexIpaImages("hd");

    // Definitive Edition uses the highest-quality compatible duplicate for the target.
    // PSP deliberately prefers phone/mobile assets to avoid wasting its tiny memory budget.
    // Desktop and Xbox prefer HD assets, then phone, then Flash/Expansion.
    if (options.gameEdition == "Definitive Edition") {
        report.definitiveEdition = true;
        addFeature(report, "Definitive Edition asset merger");
        addFeature(report, options.targetPlatform == "PSP" ? "PSP mobile-first asset policy" : "HD-first asset policy");
    }

    if(options.extractTextures){emitLog("[Importer] Converting textures...");const size_t total=parser.images().size();for(size_t i=0;i<total;++i){const auto& img=parser.images()[i];std::string baseId=AssetConverter::normalizeIdentifier(img.className,"tex",img.characterId);std::string ext=img.format==swf::ImageFormat::JPEG?".jpg":".bmp";std::string fileName=baseId+ext;fs::path filePath=texturesDir/fileName;if(AssetConverter::saveImage(img,filePath.string())){++report.texturesExtracted;const std::string relative="textures/"+fileName;addManifestEntry(textureManifestEntries,textureIds,baseId,relative);const std::string alias=nativeAssetAlias(img.className);if(!alias.empty()){addManifestEntry(textureManifestEntries,textureIds,alias,relative);addFeature(report,"Native gameplay asset aliases");}}if(progressCallback&&(i%20==0||i+1==total)){float p=total==0 ? 0.8f : 0.5f+0.3f*(float)(i+1)/(float)total;progressCallback(p,"Exporting textures ("+std::to_string(i+1)+"/"+std::to_string(total)+")...");}}emitLog("[Importer] Extracted "+std::to_string(report.texturesExtracted)+" textures.");}std::vector<std::string> audioManifestEntries;std::unordered_set<std::string> audioIds;if(options.extractAudio){emitLog("[Importer] Converting audio...");const size_t total=parser.sounds().size();for(size_t i=0;i<total;++i){const auto& snd=parser.sounds()[i];std::string baseId=AssetConverter::normalizeIdentifier(snd.className,"snd",snd.characterId);std::string ext=snd.format==swf::SoundFormat::MP3?".mp3":".wav";std::string fileName=baseId+ext;fs::path filePath=audioDir/fileName;if(AssetConverter::saveSound(snd,filePath.string())){++report.soundsExtracted;addManifestEntry(audioManifestEntries,audioIds,baseId,"audio/"+fileName);}if(progressCallback&&(i%10==0||i+1==total)){float p=total==0 ? 0.95f : 0.8f+0.15f*(float)(i+1)/(float)total;progressCallback(p,"Exporting audio ("+std::to_string(i+1)+"/"+std::to_string(total)+")...");}}emitLog("[Importer] Extracted "+std::to_string(report.soundsExtracted)+" audio cues.");}if(options.generateManifest){emitLog("[Importer] Generating manifest.json...");fs::path manifestFile=outDir/"manifest.json";std::ofstream mf(manifestFile);if(!mf.is_open()){report.errorMessage="Unable to create manifest: "+manifestFile.string();emitLog("[Importer Error] "+report.errorMessage);return report;}mf<<"{\n"<<"  \"version\": 2,\n"<<"  \"package_name\": \"Bloons TD 4 Game Data\",\n"<<"  \"source\": \""<<escapeJsonString(options.sourceSwf)<<"\",\n"<<"  \"source_family\": \""<<report.sourceFamily<<"\",\n"<<"  \"btd4_detected\": "<<(report.btd4Detected?"true":"false")<<",\n"<<"  \"swf_version\": "<<(unsigned)report.swfVersion<<",\n"<<"  \"target_platform\": \""<<options.targetPlatform<<"\",\n"<<"  \"ipa_detected\": "<<(report.ipaDetected?"true":"false")<<",\n"<<"  \"ipa_archive_detected\": "<<(report.ipaArchiveDetected?"true":"false")<<",\n"<<"  \"ipa_files_extracted\": "<<report.ipaFilesExtracted<<",\n"<<"  \"ipa_bytes_extracted\": "<<report.ipaBytesExtracted<<",\n"<<"  \"ipa_output_directory\": \"" <<escapeJsonString(report.ipaOutputDirectory)<<"\",\n"<<"  \"detected_features\": [\n";for(size_t i=0;i<report.detectedFeatures.size();++i){mf<<"    \""<<report.detectedFeatures[i]<<"\""<<(i+1==report.detectedFeatures.size()?"":",")<<"\n";}mf<<"  ],\n  \"textures\": {\n";for(size_t i=0;i<textureManifestEntries.size();++i)mf<<textureManifestEntries[i]<<(i+1==textureManifestEntries.size()?"":",")<<"\n";mf<<"  },\n  \"audio\": {\n";for(size_t i=0;i<audioManifestEntries.size();++i)mf<<audioManifestEntries[i]<<(i+1==audioManifestEntries.size()?"":",")<<"\n";mf<<"  },\n  \"maps\": [],\n  \"rounds\": null\n}\n";mf.close();report.manifestPath=manifestFile.string();emitLog("[Importer] Manifest written: "+report.manifestPath);}if(progressCallback)progressCallback(1.0f,"Import complete.");report.success=true;emitLog("[Importer] Import completed successfully. Ready for "+options.targetPlatform+".");return report;}
}
