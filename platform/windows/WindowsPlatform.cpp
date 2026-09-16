#include "WindowsPlatform.hpp"
#include <cstdlib>
#include <filesystem>
#include <sstream>

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
#ifdef _WIN32
    return "\"" + path.string() + "\"";
#else
    return "\"" + path.string() + "\"";
#endif
}

BuildResult runCommand(const std::string& command, const std::string& label) {
    BuildResult result;
    result.outputLogs.push_back("[Windows] " + label + ": " + command);
    const int code = std::system(command.c_str());
    if (code != 0) {
        result.message = label + " failed with exit code " + std::to_string(code) + ".";
        result.outputLogs.push_back("[Windows Error] " + result.message);
        return result;
    }
    result.success = true;
    result.message = label + " completed successfully.";
    return result;
}

fs::path sourceRoot() {
    return findSourceRoot();
}

fs::path buildRoot() {
    const fs::path root = sourceRoot();
    return root / "builds" / "Windows";
}

} // namespace

bool WindowsPlatform::isAvailable() const {
#ifdef _WIN32
    return true;
#else
    return std::system("command -v x86_64-w64-mingw32-g++ > /dev/null 2>&1") == 0 &&
           std::system("command -v cmake > /dev/null 2>&1") == 0;
#endif
}

BuildResult WindowsPlatform::configure() {
    BuildResult result;
    const fs::path root = sourceRoot();
    if (root.empty()) {
        result.message = "Could not locate the BTD4 Repopped source root (CMakeLists.txt).";
        return result;
    }
    if (!isAvailable()) {
        result.message = "Windows toolchain (MSVC or MinGW) not detected.";
        return result;
    }

    std::error_code ec;
    fs::create_directories(buildRoot(), ec);
    if (ec) {
        result.message = "Could not create Windows build directory: " + ec.message();
        return result;
    }

    const fs::path cmakeDir = buildRoot() / "cmake";
    std::string command = "cmake -S " + quote(root) + " -B " + quote(cmakeDir) +
                          " -DBUILD_GAME=ON -DBUILD_BUILDER=OFF -DBUILD_TESTS=OFF";
    return runCommand(command, "CMake configuration");
}

BuildResult WindowsPlatform::build() {
    const fs::path root = sourceRoot();
    const fs::path cmakeDir = buildRoot() / "cmake";
    if (root.empty() || !fs::exists(cmakeDir)) {
        BuildResult result;
        result.message = "Windows build is not configured. Run configuration first.";
        return result;
    }

    std::string command = "cmake --build " + quote(cmakeDir) + " --config Release --parallel";
    return runCommand(command, "Game compilation");
}

BuildResult WindowsPlatform::package() {
    BuildResult result;
    const fs::path root = sourceRoot();
    const fs::path cmakeDir = buildRoot() / "cmake";
    const fs::path executable = cmakeDir / "Release" / "btd4_game.exe";
    const fs::path packageDir = buildRoot() / "Playable";
    const fs::path dataSource = root / "game_data" / "Windows";

    if (root.empty() || !fs::exists(executable)) {
        result.message = "Windows executable was not produced: " + executable.string();
        return result;
    }

    std::error_code ec;
    fs::remove_all(packageDir, ec);
    fs::create_directories(packageDir, ec);
    if (ec) {
        result.message = "Could not create Windows package directory: " + ec.message();
        return result;
    }

    fs::copy_file(executable, packageDir / executable.filename(), fs::copy_options::overwrite_existing, ec);
    if (ec) {
        result.message = "Could not copy Windows executable: " + ec.message();
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
        result.outputLogs.push_back("[Windows Warning] No imported game_data/Windows directory was found; packaged game will use runtime fallbacks.");
    }

    // SDL2 is a runtime dependency on Windows when the shared SDL target is used.
    // Copy it beside the executable when CMake produced it in the build tree.
    const fs::path possibleDlls[] = {
        cmakeDir / "SDL2.dll",
        cmakeDir / "Release" / "SDL2.dll"
    };
    for (const auto& dll : possibleDlls) {
        if (fs::exists(dll, ec)) {
            fs::copy_file(dll, packageDir / dll.filename(), fs::copy_options::overwrite_existing, ec);
            if (!ec) {
                result.outputLogs.push_back("[Windows] Packaged " + dll.filename().string());
                break;
            }
        }
    }

    result.success = true;
    result.message = "Playable Windows build packaged at " + packageDir.string();
    result.outputLogs.push_back("[Windows] Executable: " + (packageDir / executable.filename()).string());
    result.outputLogs.push_back("[Windows] Imported data: " + (packageDir / "game_data").string());
    return result;
}

} // namespace btd4
