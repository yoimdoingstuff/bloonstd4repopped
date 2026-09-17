#include "AssetManifest.hpp"
#include <sstream>
#include <cctype>

namespace btd4 {

namespace {

std::string extractStringValue(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";

    size_t colon = json.find(':', pos + needle.size());
    if (colon == std::string::npos) return "";

    size_t q1 = json.find('"', colon + 1);
    if (q1 == std::string::npos) return "";

    size_t q2 = json.find('"', q1 + 1);
    if (q2 == std::string::npos) return "";

    return json.substr(q1 + 1, q2 - q1 - 1);
}

void parseDictionary(const std::string& json, const std::string& objKey, std::unordered_map<std::string, std::string>& outMap) {
    std::string needle = "\"" + objKey + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return;

    size_t braceOpen = json.find('{', pos + needle.size());
    if (braceOpen == std::string::npos) return;

    size_t braceClose = json.find('}', braceOpen + 1);
    if (braceClose == std::string::npos) return;

    std::string block = json.substr(braceOpen + 1, braceClose - braceOpen - 1);
    size_t cur = 0;
    while (cur < block.size()) {
        size_t k1 = block.find('"', cur);
        if (k1 == std::string::npos) break;
        size_t k2 = block.find('"', k1 + 1);
        if (k2 == std::string::npos) break;
        std::string key = block.substr(k1 + 1, k2 - k1 - 1);

        size_t colon = block.find(':', k2 + 1);
        if (colon == std::string::npos) break;

        size_t v1 = block.find('"', colon + 1);
        if (v1 == std::string::npos) break;
        size_t v2 = block.find('"', v1 + 1);
        if (v2 == std::string::npos) break;
        std::string val = block.substr(v1 + 1, v2 - v1 - 1);

        outMap[key] = val;
        cur = v2 + 1;
    }
}

void parseStringArray(const std::string& json, const std::string& key, std::vector<std::string>& out) {
    const std::string needle = "\"" + key + "\"";
    const size_t keyPos = json.find(needle);
    if (keyPos == std::string::npos) return;

    const size_t open = json.find('[', keyPos + needle.size());
    if (open == std::string::npos) return;
    const size_t close = json.find(']', open + 1);
    if (close == std::string::npos) return;

    size_t cur = open + 1;
    while (cur < close) {
        const size_t q1 = json.find('"', cur);
        if (q1 == std::string::npos || q1 >= close) break;
        const size_t q2 = json.find('"', q1 + 1);
        if (q2 == std::string::npos || q2 > close) break;
        const std::string value = json.substr(q1 + 1, q2 - q1 - 1);
        if (!value.empty()) out.push_back(value);
        cur = q2 + 1;
    }
}

} // namespace

bool AssetManifest::loadFromFile(const IFileSystem& fs, const std::string& manifestPath, std::string& outError) {
    std::vector<uint8_t> buffer;
    if (!fs.readFile(manifestPath, buffer)) {
        outError = "Could not read manifest file: " + manifestPath;
        return false;
    }
    std::string jsonStr(buffer.begin(), buffer.end());
    return parseJson(jsonStr, outError);
}

bool AssetManifest::parseJson(const std::string& json, std::string& outError) {
    if (json.empty()) {
        outError = "Manifest JSON is empty";
        return false;
    }

    packageName = extractStringValue(json, "package_name");
    source = extractStringValue(json, "source");
    roundsFile = extractStringValue(json, "rounds");

    textures.clear();
    audio.clear();
    maps.clear();
    parseDictionary(json, "textures", textures);
    parseDictionary(json, "audio", audio);
    parseStringArray(json, "maps", maps);

    return true;
}

std::string AssetManifest::serialize() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": " << version << ",\n";
    ss << "  \"package_name\": \"" << packageName << "\",\n";
    ss << "  \"source\": \"" << source << "\",\n";
    ss << "  \"textures\": {\n";
    size_t i = 0;
    for (const auto& [k, v] : textures) {
        ss << "    \"" << k << "\": \"" << v << "\"";
        if (++i < textures.size()) ss << ",";
        ss << "\n";
    }
    ss << "  },\n";
    ss << "  \"audio\": {\n";
    i = 0;
    for (const auto& [k, v] : audio) {
        ss << "    \"" << k << "\": \"" << v << "\"";
        if (++i < audio.size()) ss << ",";
        ss << "\n";
    }
    ss << "  },\n";
    ss << "  \"maps\": [\n";
    for (size_t m = 0; m < maps.size(); ++m) {
        ss << "    \"" << maps[m] << "\"";
        if (m + 1 < maps.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";
    ss << "  \"rounds\": \"" << roundsFile << "\"\n";
    ss << "}\n";
    return ss.str();
}

} // namespace btd4
