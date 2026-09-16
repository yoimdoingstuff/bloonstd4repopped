#include "Project.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace btd4 {

namespace fs = std::filesystem;

Project::Project() = default;

static std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

static std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";
    size_t colonPos = json.find(':', keyPos + searchKey.length());
    if (colonPos == std::string::npos) return "";
    size_t quoteStart = json.find('"', colonPos + 1);
    if (quoteStart == std::string::npos) return "";
    size_t quoteEnd = json.find('"', quoteStart + 1);
    if (quoteEnd == std::string::npos) return "";
    return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
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

static bool hasExtension(const fs::path& path, const char* wanted) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == wanted;
}

std::string Project::serialize() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": " << m_config.version << ",\n";
    ss << "  \"project_name\": \"" << m_config.projectName << "\",\n";
    ss << "  \"source_directory\": \"" << m_config.sourceDirectory << "\",\n";
    ss << "  \"source_swf\": \"" << m_config.sourceSwf << "\",\n";
    ss << "  \"source_ipa\": \"" << m_config.sourceIpa << "\",\n";
    ss << "  \"game_edition\": \"" << m_config.gameEdition << "\",\n";
    ss << "  \"enable_mobile_content\": " << (m_config.enableMobileContent ? "true" : "false") << ",\n";
    ss << "  \"target_platform\": \"" << m_config.targetPlatform << "\",\n";
    ss << "  \"build_configuration\": \"" << m_config.buildConfiguration << "\"\n";
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
    m_config.sourceSwf.clear();
    m_config.sourceIpa.clear();
    if (directory.empty()) return false;

    std::error_code ec;
    const fs::path root(directory);
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) return false;

    std::vector<fs::path> candidates;
    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
         it != end && !ec; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        if (hasExtension(it->path(), ".swf") || hasExtension(it->path(), ".ipa")) candidates.push_back(it->path());
    }
    std::sort(candidates.begin(), candidates.end());
    for (const auto& candidate : candidates) addSourceFile(candidate.string());
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
        if (m_config.sourceSwf.empty()) m_config.sourceSwf = value;
        return true;
    }
    if (hasExtension(path, ".ipa")) {
        if (m_config.sourceIpa.empty()) m_config.sourceIpa = value;
        return true;
    }
    return false;
}

bool Project::hasValidSwf() const {
    return !m_config.sourceSwf.empty() && hasExtension(fs::path(m_config.sourceSwf), ".swf") && fs::exists(m_config.sourceSwf);
}

bool Project::hasValidIpa() const {
    return !m_config.sourceIpa.empty() && hasExtension(fs::path(m_config.sourceIpa), ".ipa") && fs::exists(m_config.sourceIpa);
}

} // namespace btd4
