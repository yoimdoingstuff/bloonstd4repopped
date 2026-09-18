#include "WindowsPlatform.hpp"
#include <cstdlib>
#include <filesystem>

namespace btd4 {
namespace fs = std::filesystem;
namespace {
bool isProjectRoot(const fs::path& candidate){
    std::error_code ec;
    return fs::is_regular_file(candidate/"CMakeLists.txt",ec) &&
           fs::is_regular_file(candidate/"engine"/"core"/"Engine.cpp",ec) &&
           fs::is_regular_file(candidate/"platform"/"common"/"PlatformRegistry.cpp",ec);
}

bool isPortableRoot(const fs::path& candidate){
    std::error_code ec;
    return fs::is_regular_file(candidate/"btd4_game.exe",ec);
}

fs::path findSourceRoot(){
    std::error_code ec;

    auto searchUpward=[&](fs::path current)->fs::path{
        current=current.lexically_normal();
        for(int i=0;i<32&&!current.empty();++i){
            if(isProjectRoot(current) || isPortableRoot(current)) return current;
            const fs::path parent=current.parent_path();
            if(parent==current)break;
            current=parent;
        }
        return {};
    };

    if(const char* configuredRoot=std::getenv("BTD4_SOURCE_ROOT")){
        if(*configuredRoot){
            const fs::path root(configuredRoot);
            if(isProjectRoot(root) || isPortableRoot(root)) return root;
        }
    }

    if(const char* builderRoot=std::getenv("BTD4_BUILDER_ROOT")){
        if(*builderRoot){
            const fs::path found=searchUpward(fs::path(builderRoot));
            if(!found.empty()) return found;
        }
    }

    const fs::path current=fs::current_path(ec);
    if(!ec){
        const fs::path found=searchUpward(current);
        if(!found.empty()) return found;
    }

    return {};
}
std::string quote(const fs::path& p){return "\""+p.string()+"\"";}
BuildResult runCommand(const std::string& command,const std::string& label){BuildResult r;r.outputLogs.push_back("[Windows] "+label+": "+command);const int code=std::system(command.c_str());if(code!=0){r.message=label+" failed with exit code "+std::to_string(code)+".";r.outputLogs.push_back("[Windows Error] "+r.message);return r;}r.success=true;r.message=label+" completed successfully.";return r;}
fs::path sourceRoot(){return findSourceRoot();}
fs::path buildRoot(){return sourceRoot()/"builds"/"Windows";}
fs::path portableExecutable(){
    const fs::path root=sourceRoot();
    if (root.empty()) return {};
    const fs::path candidate=root/"btd4_game.exe";
    std::error_code ec;
    return fs::is_regular_file(candidate,ec) ? candidate : fs::path{};
}
fs::path locateExecutable(const fs::path& cmakeDir){std::error_code ec;const fs::path candidates[]={cmakeDir/"btd4_game.exe",cmakeDir/"Release"/"btd4_game.exe",cmakeDir/"Debug"/"btd4_game.exe"};for(const auto& candidate:candidates)if(fs::is_regular_file(candidate,ec))return candidate;return {};}
bool containsImportedTexture(const fs::path& textureRoot){std::error_code ec;if(!fs::is_directory(textureRoot,ec))return false;for(fs::recursive_directory_iterator it(textureRoot,fs::directory_options::skip_permission_denied,ec),end;it!=end&&!ec;it.increment(ec))if(it->is_regular_file(ec))return true;return false;}
}
bool WindowsPlatform::isAvailable() const {
#ifdef _WIN32
    return true;
#else
    return std::system("command -v x86_64-w64-mingw32-g++ > /dev/null 2>&1")==0 && std::system("command -v cmake > /dev/null 2>&1")==0;
#endif
}
BuildResult WindowsPlatform::configure(){
    BuildResult r;
    const fs::path root=sourceRoot();
    if(root.empty()){r.message="Could not locate the BTD4 Repopped source root.";return r;}

    const fs::path prebuilt=portableExecutable();
    if(!prebuilt.empty()){
        r.success=true;
        r.message="Portable prebuilt Windows game detected; compilation is not required.";
        r.outputLogs.push_back("[Windows] Portable mode: using bundled "+prebuilt.filename().string()+".");
        return r;
    }

    if(!isAvailable()){r.message="Windows toolchain (MSVC or MinGW) not detected.";return r;}
    std::error_code ec;
    const fs::path dataRoot=root/"game_data"/"Windows";
    bool importedData=false;
    if(fs::exists(dataRoot,ec)&&fs::is_directory(dataRoot,ec)){for(fs::directory_iterator it(dataRoot,ec),end;it!=end&&!ec;it.increment(ec)){if(it->is_directory(ec)&&fs::exists(it->path()/"manifest.json",ec)){importedData=true;break;}}}
    if(!importedData){r.message="No imported Windows game data is available. Import source assets before configuring a playable Windows build.";return r;}
    fs::create_directories(buildRoot(),ec);if(ec){r.message="Could not create Windows build directory: "+ec.message();return r;}
    const fs::path cmakeDir=buildRoot()/"cmake";
    return runCommand("cmake -S "+quote(root)+" -B "+quote(cmakeDir)+" -DBUILD_GAME=ON -DBUILD_BUILDER=OFF -DBUILD_TESTS=OFF","CMake configuration");
}
BuildResult WindowsPlatform::build(){
    const fs::path root=sourceRoot();
    const fs::path prebuilt=portableExecutable();
    if(!prebuilt.empty()){
        BuildResult r;
        r.success=true;
        r.message="Portable bundled Windows game is ready; compilation skipped.";
        r.outputLogs.push_back("[Windows] Portable mode: no compiler or CMake required.");
        return r;
    }

    const fs::path cmakeDir=buildRoot()/"cmake";
    if(root.empty()||!fs::exists(cmakeDir)){BuildResult r;r.message="Windows build is not configured. Run configuration first.";return r;}
    std::error_code ec;const fs::path dataRoot=root/"game_data"/"Windows";bool importedData=false;if(fs::exists(dataRoot,ec)&&fs::is_directory(dataRoot,ec)){for(fs::directory_iterator it(dataRoot,ec),end;it!=end&&!ec;it.increment(ec)){if(it->is_directory(ec)&&fs::exists(it->path()/"manifest.json",ec)){importedData=true;break;}}}if(!importedData){BuildResult r;r.message="No imported Windows game data is available. Import source assets before building.";return r;}return runCommand("cmake --build "+quote(cmakeDir)+" --config Release --parallel","Game compilation");
}
BuildResult WindowsPlatform::package(const std::string& gameEdition){
    BuildResult r;
    const fs::path root=sourceRoot();
    const fs::path prebuilt=portableExecutable();
    const fs::path cmakeDir=buildRoot()/"cmake";
    const fs::path executable=prebuilt.empty()?locateExecutable(cmakeDir):prebuilt;
    const fs::path packageDir=prebuilt.empty()?(buildRoot()/"Playable"):(root/"Playable");
    const fs::path dataRoot=root/"game_data"/"Windows";

    if(root.empty()||executable.empty()){
        r.message="Windows executable was not produced. Checked portable builder root and CMake output locations under "+cmakeDir.string();
        return r;
    }

    std::error_code ec;
    fs::remove_all(packageDir,ec);
    fs::create_directories(packageDir,ec);
    if(ec){r.message="Could not create Windows package directory: "+ec.message();return r;}

    fs::copy_file(executable,packageDir/executable.filename(),fs::copy_options::overwrite_existing,ec);
    if(ec){r.message="Could not copy Windows executable: "+ec.message();return r;}

    const fs::path packageData=packageDir/"game_data";
    fs::create_directories(packageData,ec);
    const fs::path selectedData=dataRoot/gameEdition;
    if(!gameEdition.empty()&&fs::exists(selectedData/"manifest.json",ec)){
        fs::copy(selectedData,packageData,fs::copy_options::recursive|fs::copy_options::overwrite_existing,ec);
        if(ec){r.message="Could not copy selected edition data: "+ec.message();return r;}
        r.outputLogs.push_back("[Windows] Packaged selected edition contents into game_data: "+gameEdition);
    }else if(fs::exists(dataRoot,ec)){
        r.message=gameEdition.empty()?"No game edition was selected for packaging.":"Selected game edition has no imported manifest: "+gameEdition;
        r.outputLogs.push_back("[Windows Error] "+r.message);return r;
    }else{
        r.outputLogs.push_back("[Windows Warning] No imported game_data/Windows directory was found; packaged game will use runtime fallbacks.");
    }

    if(!fs::is_regular_file(packageData/"manifest.json",ec)){
        r.message="Playable package is missing game_data/manifest.json after packaging.";
        r.outputLogs.push_back("[Windows Error] "+r.message);return r;
    }
    if(!containsImportedTexture(packageData/"textures")){
        r.message="Playable package contains no imported texture files under game_data/textures.";
        r.outputLogs.push_back("[Windows Error] "+r.message);return r;
    }

    const fs::path customUpgrades = root / "upgrades";
    if (fs::is_directory(customUpgrades, ec)) {
        const fs::path packageUpgrades = packageData / "upgrades";
        fs::create_directories(packageUpgrades, ec);
        if (ec) {
            r.message = "Could not create packaged custom upgrade directory: " + ec.message();
            return r;
        }
        fs::copy(customUpgrades, packageUpgrades,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            r.message = "Could not package custom upgrades: " + ec.message();
            return r;
        }
        r.outputLogs.push_back("[Windows] Packaged custom upgrades from " + customUpgrades.string());
    }

    const fs::path customTowers = root / "towers";
    if (fs::is_directory(customTowers, ec)) {
        const fs::path packageTowers = packageData / "towers";
        fs::create_directories(packageTowers, ec);
        if (ec) {
            r.message = "Could not create packaged custom tower directory: " + ec.message();
            return r;
        }
        fs::copy(customTowers, packageTowers,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            r.message = "Could not package custom towers: " + ec.message();
            return r;
        }
        r.outputLogs.push_back("[Windows] Packaged custom towers from " + customTowers.string());
    }

    const fs::path customRounds = root / "rounds";
    if (fs::is_directory(customRounds, ec)) {
        const fs::path packageRounds = packageData / "rounds";
        fs::create_directories(packageRounds, ec);
        if (ec) {
            r.message = "Could not create packaged custom round directory: " + ec.message();
            return r;
        }
        fs::copy(customRounds, packageRounds,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            r.message = "Could not package custom rounds: " + ec.message();
            return r;
        }
        r.outputLogs.push_back("[Windows] Packaged custom rounds from " + customRounds.string());
    }

    const fs::path customMaps = root / "maps";
    if (fs::is_directory(customMaps, ec)) {
        const fs::path packageMaps = packageData / "maps";
        fs::create_directories(packageMaps, ec);
        if (ec) {
            r.message = "Could not create packaged custom map directory: " + ec.message();
            return r;
        }
        fs::copy(customMaps, packageMaps,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            r.message = "Could not package custom maps: " + ec.message();
            return r;
        }
        r.outputLogs.push_back("[Windows] Packaged custom maps from " + customMaps.string());
    }

    const fs::path fallbackAssets = packageDir / "assets" / "placeholder";
    const fs::path sourceFallbackAssets = root / "assets" / "placeholder";
    if(fs::exists(sourceFallbackAssets, ec)){
        fs::create_directories(fallbackAssets, ec);
        if(ec){r.message="Could not create fallback asset directory: "+ec.message();return r;}
        fs::copy(sourceFallbackAssets, fallbackAssets,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if(ec){r.message="Could not package fallback assets: "+ec.message();return r;}
        r.outputLogs.push_back("[Windows] Packaged placeholder rounds/upgrades for runtime fallback.");
    }

    const fs::path possibleDlls[]={
        root/"SDL2.dll", cmakeDir/"SDL2.dll", cmakeDir/"Release"/"SDL2.dll", cmakeDir/"Debug"/"SDL2.dll",
        root/"SDL2_image.dll", cmakeDir/"SDL2_image.dll", cmakeDir/"Release"/"SDL2_image.dll", cmakeDir/"Debug"/"SDL2_image.dll"
    };
    for(const auto& dll:possibleDlls)if(fs::exists(dll,ec)){fs::copy_file(dll,packageDir/dll.filename(),fs::copy_options::overwrite_existing,ec);if(!ec){r.outputLogs.push_back("[Windows] Packaged "+dll.filename().string());break;}}
    r.success=true;
    r.message="Playable Windows build packaged at "+packageDir.string();
    r.outputLogs.push_back("[Windows] Executable: "+(packageDir/executable.filename()).string());
    r.outputLogs.push_back("[Windows] Imported data: "+packageData.string());
    return r;
}
}
