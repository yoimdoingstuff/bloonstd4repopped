#include "Project.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <initializer_list>

namespace btd4 {

namespace fs = std::filesystem;

Project::Project() = default;

static std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

static std::string parseQuotedJsonString(const std::string& json, size_t quote) {
    if (quote >= json.size() || json[quote] != '"') return {};
    std::string value;
    for (size_t i = quote + 1; i < json.size(); ++i) {
        const char c = json[i];
        if (c == '"') return value;
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

static std::string extractJsonString(const std::string& json, const std::string& key) {
    const std::string searchKey = "\"" + key + "\"";
    const size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return {};
    const size_t colonPos = json.find(':', keyPos + searchKey.length());
    if (colonPos == std::string::npos) return {};
    const size_t quoteStart = json.find('"', colonPos + 1);
    if (quoteStart == std::string::npos) return {};
    return parseQuotedJsonString(json, quoteStart);
}

static std::string escapeJsonString(const std::string& value) {
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

static int extractJsonInt(const std::string& json, const std::string& key, int defaultVal = 0) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return defaultVal;
    size_t colonPos = json.find(':', keyPos + searchKey.length());
    if (colonPos == std::string::npos) return defaultVal;
    size_t valStart = json.find_first_of("-0123456789", colonPos + 1);
    if (valStart == std::string::npos) return defaultVal;
    size_t valEnd = json.find_first_not_of("-0123456789", valStart);
    try { return std::stoi(valEnd == std::string::npos ? json.substr(valStart) : json.substr(valStart, valEnd - valStart)); }
    catch (...) { return defaultVal; }
}

static bool extractJsonBool(const std::string& json, const std::string& key, bool defaultVal = false) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return defaultVal;
    size_t colonPos = json.find(':', keyPos + searchKey.length());
    if (colonPos == std::string::npos) return defaultVal;
    std::string rest = json.substr(colonPos + 1, 10);
    if (rest.find("true") != std::string::npos) return true;
    if (rest.find("false") != std::string::npos) return false;
    return defaultVal;
}

static std::string lowerPathName(const fs::path& path) {
    std::string value = path.filename().string();
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static bool hasExtension(const fs::path& path, const char* wanted) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == wanted;
}

static bool nameContains(const fs::path& path, std::initializer_list<const char*> tokens) {
    const std::string name = lowerPathName(path);
    for (const char* token : tokens) {
        if (name.find(token) != std::string::npos) return true;
    }
    return false;
}

std::string Project::serialize() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": " << m_config.version << ",\n";
    ss << "  \"project_name\": \"" << escapeJsonString(m_config.projectName) << "\",\n";
    ss << "  \"source_directory\": \"" << escapeJsonString(m_config.sourceDirectory) << "\",\n";
    ss << "  \"source_swf\": \"" << escapeJsonString(m_config.sourceSwf) << "\",\n";
    ss << "  \"source_ipa\": \"" << escapeJsonString(m_config.sourceIpa) << "\",\n";
    ss << "  \"source_expansion_swf\": \"" << escapeJsonString(m_config.sourceExpansionSwf) << "\",\n";
    ss << "  \"source_hd_ipa\": \"" << escapeJsonString(m_config.sourceHdIpa) << "\",\n";
    ss << "  \"source_mobile_ipa\": \"" << escapeJsonString(m_config.sourceMobileIpa) << "\",\n";
    ss << "  \"game_edition\": \"" << escapeJsonString(m_config.gameEdition) << "\",\n";
    ss << "  \"enable_mobile_content\": " << (m_config.enableMobileContent ? "true" : "false") << ",\n";
    ss << "  \"target_platform\": \"" << escapeJsonString(m_config.targetPlatform) << "\",\n";
    ss << "  \"build_configuration\": \"" << escapeJsonString(m_config.buildConfiguration) << "\"\n";
    ss << "}\n";
    return ss.str();
}

bool Project::deserialize(const std::string& json) {
    if (json.empty()) return false;
    m_config.version = extractJsonInt(json, "version", 1);
    std::string name = extractJsonString(json, "project_name"); if (!name.empty()) m_config.projectName = name;
    std::string sourceDirectory = extractJsonString(json, "source_directory"); if (!sourceDirectory.empty()) m_config.sourceDirectory = sourceDirectory;
    m_config.sourceSwf = extractJsonString(json, "source_swf");
    m_config.sourceIpa = extractJsonString(json, "source_ipa");
    m_config.sourceExpansionSwf = extractJsonString(json, "source_expansion_swf");
    m_config.sourceHdIpa = extractJsonString(json, "source_hd_ipa");
    m_config.sourceMobileIpa = extractJsonString(json, "source_mobile_ipa");
    std::string edition = extractJsonString(json, "game_edition"); if (!edition.empty()) m_config.gameEdition = edition;
    m_config.enableMobileContent = extractJsonBool(json, "enable_mobile_content", false);
    std::string plat = extractJsonString(json, "target_platform"); if (!plat.empty()) m_config.targetPlatform = plat;
    std::string config = extractJsonString(json, "build_configuration"); if (!config.empty()) m_config.buildConfiguration = config;
    return true;
}

bool Project::loadFromFile(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return false;
    std::stringstream buffer; buffer << in.rdbuf();
    return deserialize(buffer.str());
}

bool Project::saveToFile(const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    out << serialize();
    return true;
}

bool Project::discoverSourceAssets() { return discoverSourceAssets(m_config.sourceDirectory); }

bool Project::discoverSourceAssets(const std::string& directory) {
    m_config.sourceDirectory = directory;
    if (directory.empty()) return false;

    std::error_code ec;
    const fs::path root(directory);
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) return false;

    std::vector<fs::path> swfCandidates;
    std::vector<fs::path> ipaCandidates;
    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
         it != end && !ec; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        if (hasExtension(it->path(), ".swf")) swfCandidates.push_back(it->path());
        else if (hasExtension(it->path(), ".ipa")) ipaCandidates.push_back(it->path());
    }
    std::sort(swfCandidates.begin(), swfCandidates.end());
    std::sort(ipaCandidates.begin(), ipaCandidates.end());

    if (swfCandidates.empty() && ipaCandidates.empty()) {
        return false;
    }

    m_config.sourceSwf.clear();
    m_config.sourceIpa.clear();
    m_config.sourceExpansionSwf.clear();
    m_config.sourceHdIpa.clear();
    m_config.sourceMobileIpa.clear();

    // Prefer the non-expansion SWF as the base game. A plain lexical sort can
    // put "Expansion" before the normal BTD4 SWF.
    for (const auto& candidate : swfCandidates) {
        if (!nameContains(candidate, {"expansion", "exp"})) {
            addSourceFile(candidate.string());
            if (!m_config.sourceSwf.empty()) break;
        }
    }
    for (const auto& candidate : swfCandidates) {
        if (nameContains(candidate, {"expansion", "exp"})) {
            addSourceFile(candidate.string());
            if (!m_config.sourceExpansionSwf.empty()) break;
        }
    }
    for (const auto& candidate : swfCandidates) {
        addSourceFile(candidate.string());
    }

    // Use filename hints for mobile packages when available. Otherwise keep
    // the first generic IPA as the legacy/primary mobile source.
    for (const auto& candidate : ipaCandidates) {
        if (nameContains(candidate, {"hd", "ipad", "tablet"})) addSourceFile(candidate.string());
    }
    for (const auto& candidate : ipaCandidates) {
        if (nameContains(candidate, {"mobile", "phone", "iphone"})) addSourceFile(candidate.string());
    }
    for (const auto& candidate : ipaCandidates) {
        if (!nameContains(candidate, {"hd", "ipad", "tablet", "mobile", "phone", "iphone"})) {
            addSourceFile(candidate.string());
        }
    }

    return !m_config.sourceSwf.empty();
}

bool Project::addSourceFile(const std::string& filepath) {
    if (filepath.empty()) return false;
    fs::path path(filepath);
    std::error_code ec;
    if (!fs::exists(path, ec) || !fs::is_regular_file(path, ec)) return false;

    const fs::path absolute = fs::absolute(path, ec);
    const std::string value = (ec ? path : absolute).lexically_normal().string();
    if (hasExtension(path, ".swf")) {
        if (nameContains(path, {"expansion", "exp"}) && m_config.sourceExpansionSwf.empty()) {
            m_config.sourceExpansionSwf = value;
        } else if (m_config.sourceSwf.empty()) {
            m_config.sourceSwf = value;
        } else if (m_config.sourceExpansionSwf.empty()) {
            m_config.sourceExpansionSwf = value;
        } else {
            return false;
        }
        return true;
    }
    if (hasExtension(path, ".ipa")) {
        if (nameContains(path, {"hd", "ipad", "tablet"}) && m_config.sourceHdIpa.empty()) {
            m_config.sourceHdIpa = value;
        } else if (nameContains(path, {"mobile", "phone", "iphone"}) && m_config.sourceMobileIpa.empty()) {
            m_config.sourceMobileIpa = value;
        } else if (m_config.sourceIpa.empty()) {
            m_config.sourceIpa = value;
        } else if (m_config.sourceHdIpa.empty()) {
            m_config.sourceHdIpa = value;
        } else if (m_config.sourceMobileIpa.empty()) {
            m_config.sourceMobileIpa = value;
        } else {
            return false;
        }
        return true;
    }
    return false;
}

// A project file stores source locations independently of whether the original
// files are currently mounted. This lets a project deserialize and display its
// configured sources without silently rewriting those paths. The actual import
// path validates file existence before parsing.
bool Project::hasValidSwf() const {
    return !m_config.sourceSwf.empty() && hasExtension(fs::path(m_config.sourceSwf), ".swf");
}

bool Project::hasValidIpa() const {
    return !m_config.sourceIpa.empty() && hasExtension(fs::path(m_config.sourceIpa), ".ipa");
}

} // namespace btd4