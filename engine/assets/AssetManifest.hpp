#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "../core/IFileSystem.hpp"

namespace btd4 {

struct AssetManifest {
    int version{1};
    std::string packageName;
    std::string source;
    std::string targetPlatform;
    std::unordered_map<std::string, std::string> textures;
    std::unordered_map<std::string, std::string> audio;
    std::vector<std::string> maps;
    std::string roundsFile;

    bool loadFromFile(const IFileSystem& fs, const std::string& manifestPath, std::string& outError);
    bool parseJson(const std::string& json, std::string& outError);
    std::string serialize() const;
};

} // namespace btd4
