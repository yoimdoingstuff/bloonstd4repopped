#include "LinuxPlatform.hpp"
#include <cstdlib>
#include <filesystem>
#include <string>

namespace btd4 {
namespace fs = std::filesystem;

namespace {
fs::path findSourceRoot() {
    std::error_code ec;
    fs::path current = fs::current_path(ec);
    for (int i = 0; i < 8 && !current.empty(); ++i) {
        if (fs::exists(current / "CMakeLists.txt", ec)) return current;
        const fs::path parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return {};
}

std::string quote(const fs::path& path) {
    return "\"" + path.string() + "\"";
}

BuildResult runCommand(const std::string& command, const std::string& label) {
    BuildResult result;
    result.outputLogs.push_back("[Linux] " + label + ": " + command);
    const int code = std::system(command.c_str());
    if (code != 0) {
        result.message = label + " failed with exit code " + std::to_string(code) + ".";
        result.outputLogs.push_back("[Linux Error] " + result.message);
        return result;
    }
    result.success = true;
    result.message = label + " completed successfully.";
    return result;
}

fs::path buildRoot() { return findSourceRoot() / "builds" / "Linux"; }

bool executableOnPath(const char* executable) {
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv || !*pathEnv || !executable || !*executable) return false;

    std::string pathList(pathEnv);
    size_t start = 0;
    while (start <= pathList.size()) {
        const size_t end = pathList.find(':', start);
        const std::string entry = pathList.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!entry.empty()) {
            std::error_code ec;
            if (fs::is_regular_file(fs::path(entry) / executable, ec)) return true;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return false;
}

bool containsImportedTexture(const fs::path& textureRoot) {
    std::error_code ec;
    if (!fs::is_directory(textureRoot, ec)) return false;
    for (fs::recursive_directory_iterator it(textureRoot, fs::directory_options::skip_permission_denied, ec), end;
         it != end && !ec; it.increment(ec)) {
        if (it->is_regular_file(ec)) return true;
    }
    return false;
}

bool LinuxPlatform::isAvailable() const {
#if defined(__linux__)
    static const bool available = [] {
        return (executableOnPath("g++") || executableOnPath("clang++")) && executableOnPath("cmake");
    }();
    return available;
#else
    return false;
#endif
}

BuildResult LinuxPlatform::configure() {
    BuildResult result;
    const fs::path root = findSourceRoot();
    if (root.empty()) {
        result.message = "Could not locate the BTD4 Repopped source root (CMakeLists.txt).";
        return result;
    }
    if (!isAvailable()) {
        result.message = "Linux toolchain (g++/clang++ and cmake) not found.";
        return result;
    }

    std::error_code ec;
    const fs::path dataRoot = root / "game_data" / "Linux";
    bool importedData = false;
    if (fs::exists(dataRoot, ec) && fs::is_directory(dataRoot, ec)) {
        for (fs::directory_iterator it(dataRoot, ec), end; it != end && !ec; it.increment(ec)) {
            if (it->is_directory(ec) && fs::exists(it->path() / "manifest.json", ec)) {
                importedData = true;
                break;
            }
        }
    }
    if (!importedData) {
        result.message = "No imported Linux game data is available. Import source assets before configuring a playable Linux build.";
        return result;
    }

    fs::create_directories(buildRoot(), ec);
    if (ec) {
        result.message = "Could not create Linux build directory: " + ec.message();
        return result;
    }

    const fs::path cmakeDir = buildRoot() / "cmake";
    return runCommand("cmake -S " + quote(root) + " -B " + quote(cmakeDir) +
                      " -DBUILD_GAME=ON -DBUILD_BUILDER=OFF -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release",
                      "CMake configuration");
}

BuildResult LinuxPlatform::build() {
    BuildResult result;
    const fs::path root = findSourceRoot();
    const fs::path cmakeDir = buildRoot() / "cmake";
    if (root.empty() || !fs::exists(cmakeDir)) {
        result.message = "Linux build is not configured. Run configuration first.";
        return result;
    }

    std::error_code ec;
    const fs::path dataRoot = root / "game_data" / "Linux";
    bool importedData = false;
    if (fs::exists(dataRoot, ec) && fs::is_directory(dataRoot, ec)) {
        for (fs::directory_iterator it(dataRoot, ec), end; it != end && !ec; it.increment(ec)) {
            if (it->is_directory(ec) && fs::exists(it->path() / "manifest.json", ec)) {
                importedData = true;
                break;
            }
        }
    }
    if (!importedData) {
        result.message = "No imported Linux game data is available. Import source assets before building.";
        return result;
    }

    return runCommand("cmake --build " + quote(cmakeDir) + " --config Release --parallel", "Game compilation");
}

BuildResult LinuxPlatform::package(const std::string& gameEdition) {
    BuildResult result;
    const fs::path root = findSourceRoot();
    const fs::path executable = buildRoot() / "cmake" / "btd4_game";
    const fs::path packageDir = buildRoot() / "Playable";
    const fs::path dataRoot = root / "game_data" / "Linux";

    if (root.empty() || !fs::exists(executable)) {
        result.message = "Linux executable was not produced: " + executable.string();
        return result;
    }

    std::error_code ec;
    fs::remove_all(packageDir, ec);
    fs::create_directories(packageDir, ec);
    if (ec) {
        result.message = "Could not create Linux package directory: " + ec.message();
        return result;
    }

    fs::copy_file(executable, packageDir / executable.filename(), fs::copy_options::overwrite_existing, ec);
    if (ec) {
        result.message = "Could not copy Linux executable: " + ec.message();
        return result;
    }

    const fs::path packageData = packageDir / "game_data";
    fs::create_directories(packageData, ec);
    if (!gameEdition.empty() && fs::exists(dataRoot / gameEdition / "manifest.json", ec)) {
        fs::copy(dataRoot / gameEdition, packageData,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            result.message = "Could not copy selected edition data: " + ec.message();
            return result;
        }
        result.outputLogs.push_back("[Linux] Packaged selected edition: " + gameEdition);
    } else if (fs::exists(dataRoot, ec)) {
        result.message = gameEdition.empty()
            ? "No game edition was selected for packaging."
            : "Selected game edition has no imported manifest: " + gameEdition;
        result.outputLogs.push_back("[Linux Error] " + result.message);
        return result;
    } else {
        result.outputLogs.push_back("[Linux Warning] No imported game_data/Linux directory was found; packaged game will use runtime fallbacks.");
    }

    if (!fs::is_regular_file(packageData / "manifest.json", ec)) {
        result.message = "Playable package is missing game_data/manifest.json after packaging.";
        result.outputLogs.push_back("[Linux Error] " + result.message);
        return result;
    }
    if (!containsImportedTexture(packageData / "textures")) {
        result.message = "Playable package contains no imported texture files under game_data/textures.";
        result.outputLogs.push_back("[Linux Error] " + result.message);
        return result;
    }

    const fs::path customUpgrades = root / "upgrades";
    if (fs::is_directory(customUpgrades, ec)) {
        const fs::path packageUpgrades = packageDir / "game_data" / "upgrades";
        fs::create_directories(packageUpgrades, ec);
        if (ec) {
            result.message = "Could not create packaged custom upgrade directory: " + ec.message();
            return result;
        }
        fs::copy(customUpgrades, packageUpgrades,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            result.message = "Could not package custom upgrades: " + ec.message();
            return result;
        }
        result.outputLogs.push_back("[Linux] Packaged custom upgrades from " + customUpgrades.string());
    }

    const fs::path customTowers = root / "towers";
    if (fs::is_directory(customTowers, ec)) {
        const fs::path packageTowers = packageDir / "game_data" / "towers";
        fs::create_directories(packageTowers, ec);
        if (ec) {
            result.message = "Could not create packaged custom tower directory: " + ec.message();
            return result;
        }
        fs::copy(customTowers, packageTowers,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            result.message = "Could not package custom towers: " + ec.message();
            return result;
        }
        result.outputLogs.push_back("[Linux] Packaged custom towers from " + customTowers.string());
    }

    const fs::path customRounds = root / "rounds";
    if (fs::is_directory(customRounds, ec)) {
        const fs::path packageRounds = packageDir / "game_data" / "rounds";
        fs::create_directories(packageRounds, ec);
        if (ec) {
            result.message = "Could not create packaged custom round directory: " + ec.message();
            return result;
        }
        fs::copy(customRounds, packageRounds,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            result.message = "Could not package custom rounds: " + ec.message();
            return result;
        }
        result.outputLogs.push_back("[Linux] Packaged custom rounds from " + customRounds.string());
    }

    const fs::path customMaps = root / "maps";
    if (fs::is_directory(customMaps, ec)) {
        const fs::path packageMaps = packageDir / "game_data" / "maps";
        fs::create_directories(packageMaps, ec);
        if (ec) {
            result.message = "Could not create packaged custom map directory: " + ec.message();
            return result;
        }
        fs::copy(customMaps, packageMaps,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            result.message = "Could not package custom maps: " + ec.message();
            return result;
        }
        result.outputLogs.push_back("[Linux] Packaged custom maps from " + customMaps.string());
    }

    const fs::path fallbackAssets = packageDir / "assets" / "placeholder";
    const fs::path sourceFallbackAssets = root / "assets" / "placeholder";
    if (fs::exists(sourceFallbackAssets, ec)) {
        fs::create_directories(fallbackAssets, ec);
        if (ec) {
            result.message = "Could not create fallback asset directory: " + ec.message();
            return result;
        }
        fs::copy(sourceFallbackAssets, fallbackAssets,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            result.message = "Could not package fallback assets: " + ec.message();
            return result;
        }
        result.outputLogs.push_back("[Linux] Packaged placeholder rounds/upgrades for runtime fallback.");
    }

    fs::permissions(packageDir / "btd4_game",
                    fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                    fs::perm_options::add, ec);

    result.success = true;
    result.message = "Playable Linux build packaged at " + packageDir.string();
    result.outputLogs.push_back("[Linux] Executable: " + (packageDir / "btd4_game").string());
    result.outputLogs.push_back("[Linux] Imported data: " + packageData.string());
    return result;
}

} // namespace btd4
