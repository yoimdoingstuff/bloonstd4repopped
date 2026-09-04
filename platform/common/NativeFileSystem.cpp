#include "NativeFileSystem.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace btd4 {
namespace {

namespace fs = std::filesystem;

}

bool NativeFileSystem::readFile(const std::string& path, std::vector<uint8_t>& data) const {
    data.clear();

    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        return false;
    }

    const std::streamsize size = input.tellg();
    if (size < 0) {
        return false;
    }

    data.resize(static_cast<size_t>(size));
    input.seekg(0, std::ios::beg);
    if (size == 0) {
        return true;
    }
    return input.read(reinterpret_cast<char*>(data.data()), size).good();
}

bool NativeFileSystem::writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    const fs::path target(path);
    const fs::path parent = target.parent_path();
    std::error_code error;
    if (!parent.empty() && !fs::exists(parent, error) && !fs::create_directories(parent, error)) {
        return false;
    }

    std::ofstream output(target, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }

    if (!data.empty()) {
        output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    return output.good();
}

bool NativeFileSystem::fileExists(const std::string& path) const {
    std::error_code error;
    return fs::is_regular_file(fs::path(path), error);
}

bool NativeFileSystem::createDirectories(const std::string& path) {
    std::error_code error;
    if (fs::is_directory(fs::path(path), error)) {
        return true;
    }
    return fs::create_directories(fs::path(path), error);
}

bool NativeFileSystem::removeFile(const std::string& path) {
    std::error_code error;
    return fs::remove(fs::path(path), error);
}

bool NativeFileSystem::listFiles(const std::string& path, std::vector<std::string>& files) const {
    files.clear();
    std::error_code error;
    fs::directory_iterator iterator(fs::path(path), error);
    if (error) {
        return false;
    }

    while (iterator != fs::directory_iterator()) {
        const fs::directory_entry& entry = *iterator;
        const bool isRegularFile = entry.is_regular_file(error);
        if (error) {
            files.clear();
            return false;
        }
        if (isRegularFile) {
            files.push_back(entry.path().filename().u8string());
        }
        iterator.increment(error);
        if (error) {
            files.clear();
            return false;
        }
    }
    std::sort(files.begin(), files.end());
    return true;
}

} // namespace btd4
