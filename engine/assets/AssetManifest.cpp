#include "AssetManifest.hpp"
#include <sstream>
#include <cctype>

namespace btd4 {

namespace {
std::string parseQuotedString(const std::string& json, size_t quote, size_t* nextPos = nullptr) {
    if (quote >= json.size() || json[quote] != '"') return {};
    std::string value;
    for (size_t i = quote + 1; i < json.size(); ++i) {
        const char c = json[i];
        if (c == '"') {
            if (nextPos) *nextPos = i + 1;
            return value;
        }
        if (c != '\\') {
            value += c;
            continue;
        }
        if (++i >= json.size()) return {};
        switch (json[i]) {
            case '"': value += '"'; break;
            case '\\': value += '\\'; break;
            case '/': value += '/'; break;
            case 'b': value += '\b'; break;
            case 'f': value += '\f'; break;
            case 'n': value += '\n'; break;
            case 'r': value += '\r'; break;
            case 't': value += '\t'; break;
            default: return {};
        }
    }
    return {};
}

std::string extractStringValue(const std::string& json, const std::string& key) {
    const std::string needle = "\""+key+"\"";
    const size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    const size_t colon = json.find(':', pos + needle.size());
    if (colon == std::string::npos) return "";
    const size_t quote = json.find('"', colon + 1);
    if (quote == std::string::npos) return "";
    return parseQuotedString(json, quote);
}

void parseDictionary(const std::string& json, const std::string& objKey,
                     std::unordered_map<std::string, std::string>& outMap) {
    const std::string needle = "\""+objKey+"\"";
    const size_t pos = json.find(needle);
    if (pos == std::string::npos) return;
    const size_t braceOpen = json.find('{', pos + needle.size());
    if (braceOpen == std::string::npos) return;
    const size_t braceClose = json.find('}', braceOpen + 1);
    if (braceClose == std::string::npos) return;

    size_t cur = braceOpen + 1;
    while (cur < braceClose) {
        const size_t k1 = json.find('"', cur);
        if (k1 == std::string::npos || k1 >= braceClose) break;
        size_t keyEnd = 0;
        const std::string key = parseQuotedString(json, k1, &keyEnd);
        if (key.empty() && keyEnd == 0) break;
        const size_t colon = json.find(':', keyEnd);
        if (colon == std::string::npos || colon >= braceClose) break;
        const size_t v1 = json.find('"', colon + 1);
        if (v1 == std::string::npos || v1 >= braceClose) break;
        size_t valueEnd = 0;
        const std::string value = parseQuotedString(json, v1, &valueEnd);
        if (valueEnd == 0) break;
        outMap[key] = value;
        cur = valueEnd;
    }
}

void parseStringArray(const std::string& json, const std::string& key,
                      std::vector<std::string>& out) {
    const std::string needle = "\""+key+"\"";
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
        size_t valueEnd = 0;
        const std::string value = parseQuotedString(json, q1, &valueEnd);
        if (valueEnd == 0 || valueEnd > close) break;
        if (!value.empty()) out.push_back(value);
        cur = valueEnd;
    }
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
