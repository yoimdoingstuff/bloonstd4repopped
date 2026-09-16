#include "LinuxPlatform.hpp"
#include <cstdlib>
#include <filesystem>

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
}

bool LinuxPlatform::isAvailable() const {
#if defined(__linux__)
    return (std::system("command -v g++ > /dev/null 2>&1") == 0 ||
            std::system("command -v clang++ > /dev/null 2>&1") == 0) &&
           std::system("command -v cmake > /dev/null 2>&1") == 0;
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
    const fs::path cmakeDir = buildRoot() / "cmake";
    if (cmakeDir.empty() || !fs::exists(cmakeDir)) {
        result.message = "Linux build is not configured. Run configuration first.";
        return result;
    }
    return runCommand("cmake --build " + quote(cmakeDir) + " --config Release --parallel", "Game compilation");
}

BuildResult LinuxPlatform::package() {
    BuildResult result;
    const fs::path root = findSourceRoot();
    const fs::path executable = buildRoot() / "cmake" / "btd4_game";
    const fs::path packageDir = buildRoot() / "Playable";
    const fs::path dataSource = root / "game_data" / "Linux";

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

    if (fs::exists(dataSource, ec)) {
        fs::copy(dataSource, packageDir / "game_data",
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) {
            result.message = "Could not copy imported game data: " + ec.message();
            return result;
        }
    } else {
        result.outputLogs.push_back("[Linux Warning] No imported game_data/Linux directory was found; packaged game will use runtime fallbacks.");
    }

    fs::permissions(packageDir / "btd4_game",
                    fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                    fs::perm_options::add, ec);

    result.success = true;
    result.message = "Playable Linux build packaged at " + packageDir.string();
    result.outputLogs.push_back("[Linux] Executable: " + (packageDir / "btd4_game").string());
    result.outputLogs.push_back("[Linux] Imported data: " + (packageDir / "game_data").string());
    return result;
}

} // namespace btd4
