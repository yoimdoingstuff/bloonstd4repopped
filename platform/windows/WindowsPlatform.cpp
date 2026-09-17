#include "WindowsPlatform.hpp"
#include <cstdlib>
#include <filesystem>

namespace btd4 {
namespace fs = std::filesystem;
namespace {
fs::path findSourceRoot(){std::error_code ec;fs::path current=fs::current_path(ec);for(int i=0;i<8&&!current.empty();++i){if(fs::exists(current/"CMakeLists.txt",ec))return current;const fs::path parent=current.parent_path();if(parent==current)break;current=parent;}return {};}
std::string quote(const fs::path& p){return "\""+p.string()+"\"";}
BuildResult runCommand(const std::string& command,const std::string& label){BuildResult r;r.outputLogs.push_back("[Windows] "+label+": "+command);const int code=std::system(command.c_str());if(code!=0){r.message=label+" failed with exit code "+std::to_string(code)+".";r.outputLogs.push_back("[Windows Error] "+r.message);return r;}r.success=true;r.message=label+" completed successfully.";return r;}
fs::path sourceRoot(){return findSourceRoot();}
fs::path buildRoot(){return sourceRoot()/"builds"/"Windows";}
}
bool WindowsPlatform::isAvailable() const {
#ifdef _WIN32
    return true;
#else
    return std::system("command -v x86_64-w64-mingw32-g++ > /dev/null 2>&1")==0 && std::system("command -v cmake > /dev/null 2>&1")==0;
#endif
}
BuildResult WindowsPlatform::configure(){BuildResult r;const fs::path root=sourceRoot();if(root.empty()){r.message="Could not locate the BTD4 Repopped source root (CMakeLists.txt).";return r;}if(!isAvailable()){r.message="Windows toolchain (MSVC or MinGW) not detected.";return r;}
    std::error_code ec;
    const fs::path dataRoot = root/"game_data"/"Windows";
    bool importedData = false;
    if(fs::exists(dataRoot,ec) && fs::is_directory(dataRoot,ec)){
        for(fs::directory_iterator it(dataRoot,ec),end;it!=end&&!ec;it.increment(ec)){
            if(it->is_directory(ec) && fs::exists(it->path()/"manifest.json",ec)){importedData=true;break;}
        }
    }
    if(!importedData){r.message="No imported Windows game data is available. Import source assets before configuring a playable Windows build.";return r;}
    fs::create_directories(buildRoot(),ec);if(ec){r.message="Could not create Windows build directory: "+ec.message();return r;}const fs::path cmakeDir=buildRoot()/"cmake";return runCommand("cmake -S "+quote(root)+" -B "+quote(cmakeDir)+" -DBUILD_GAME=ON -DBUILD_BUILDER=OFF -DBUILD_TESTS=OFF","CMake configuration");}
BuildResult WindowsPlatform::build(){const fs::path root=sourceRoot();const fs::path cmakeDir=buildRoot()/"cmake";if(root.empty()||!fs::exists(cmakeDir)){BuildResult r;r.message="Windows build is not configured. Run configuration first.";return r;}
    std::error_code ec;
    const fs::path dataRoot = root/"game_data"/"Windows";
    bool importedData = false;
    if(fs::exists(dataRoot,ec) && fs::is_directory(dataRoot,ec)){
        for(fs::directory_iterator it(dataRoot,ec),end;it!=end&&!ec;it.increment(ec)){
            if(it->is_directory(ec) && fs::exists(it->path()/"manifest.json",ec)){importedData=true;break;}
        }
    }
    if(!importedData){BuildResult r;r.message="No imported Windows game data is available. Import source assets before building.";return r;}
    return runCommand("cmake --build "+quote(cmakeDir)+" --config Release --parallel","Game compilation");}
BuildResult WindowsPlatform::package(const std::string& gameEdition){BuildResult r;const fs::path root=sourceRoot();const fs::path cmakeDir=buildRoot()/"cmake";const fs::path executable=cmakeDir/"Release"/"btd4_game.exe";const fs::path packageDir=buildRoot()/"Playable";const fs::path dataRoot=root/"game_data"/"Windows";if(root.empty()||!fs::exists(executable)){r.message="Windows executable was not produced: "+executable.string();return r;}std::error_code ec;fs::remove_all(packageDir,ec);fs::create_directories(packageDir,ec);if(ec){r.message="Could not create Windows package directory: "+ec.message();return r;}fs::copy_file(executable,packageDir/executable.filename(),fs::copy_options::overwrite_existing,ec);if(ec){r.message="Could not copy Windows executable: "+ec.message();return r;}
    const fs::path packageData=packageDir/"game_data";
    fs::create_directories(packageData,ec);
    if(!gameEdition.empty()&&fs::exists(dataRoot/gameEdition/"manifest.json",ec)){
        fs::copy(dataRoot/gameEdition,packageData,fs::copy_options::recursive|fs::copy_options::overwrite_existing,ec);
        if(ec){r.message="Could not copy selected edition data: "+ec.message();return r;}
        r.outputLogs.push_back("[Windows] Packaged selected edition: "+gameEdition);
    }else if(fs::exists(dataRoot,ec)){
        r.message=gameEdition.empty()?"No game edition was selected for packaging.":"Selected game edition has no imported manifest: "+gameEdition;
        r.outputLogs.push_back("[Windows Error] "+r.message);
        return r;
    }else{
        r.outputLogs.push_back("[Windows Warning] No imported game_data/Windows directory was found; packaged game will use runtime fallbacks.");
    }
    const fs::path possibleDlls[]={cmakeDir/"SDL2.dll",cmakeDir/"Release"/"SDL2.dll"};
    for(const auto& dll:possibleDlls)if(fs::exists(dll,ec)){fs::copy_file(dll,packageDir/dll.filename(),fs::copy_options::overwrite_existing,ec);if(!ec){r.outputLogs.push_back("[Windows] Packaged "+dll.filename().string());break;}}
    if(!fs::exists(packageData/"manifest.json",ec)) r.outputLogs.push_back("[Windows] No imported manifest was packaged; runtime fallback assets remain enabled.");
    r.success=true;r.message="Playable Windows build packaged at "+packageDir.string();r.outputLogs.push_back("[Windows] Executable: "+(packageDir/executable.filename()).string());r.outputLogs.push_back("[Windows] Imported data: "+packageData.string());return r;}
}
