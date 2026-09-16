#include "Project.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
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
    std::string numStr = (valEnd == std::string::npos) ? json.substr(valStart) : json.substr(valStart, valEnd - valStart);
    try {
        return std::stoi(numStr);
    } catch (...) {
        return defaultVal;
    }
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

std::string Project::serialize() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": " << m_config.version << ",\n";
    ss << "  \"project_name\": \"" << m_config.projectName << "\",\n";
    ss << "  \"source_directory\": \"" << m_config.sourceDirectory << "\",\n";
    ss << "  \"source_swf\": \"" << m_config.sourceSwf << "\",\n";
    ss << "  \"source_ipa\": \"" << m_config.sourceIpa << "\",\n";
    ss << "  \"enable_mobile_content\": " << (m_config.enableMobileContent ? "true" : "false") << ",\n";
    ss << "  \"target_platform\": \"" << m_config.targetPlatform << "\",\n";
    ss << "  \"build_configuration\": \"" << m_config.buildConfiguration << "\"\n";
    ss << "}\n";
    return ss.str();
}

bool Project::deserialize(const std::string& json) {
    if (json.empty()) return false;

    m_config.version = extractJsonInt(json, "version", 1);
    std::string name = extractJsonString(json, "project_name");
    if (!name.empty()) m_config.projectName = name;

    std::string sourceDirectory = extractJsonString(json, "source_directory");
    if (!sourceDirectory.empty()) m_config.sourceDirectory = sourceDirectory;
    m_config.sourceSwf = extractJsonString(json, "source_swf");
    m_config.sourceIpa = extractJsonString(json, "source_ipa");
    m_config.enableMobileContent = extractJsonBool(json, "enable_mobile_content", false);

    std::string plat = extractJsonString(json, "target_platform");
    if (!plat.empty()) m_config.targetPlatform = plat;

    std::string config = extractJsonString(json, "build_configuration");
    if (!config.empty()) m_config.buildConfiguration = config;

    return true;
}

bool Project::loadFromFile(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return false;

    std::stringstream buffer;
    buffer << in.rdbuf();
    return deserialize(buffer.str());
}

bool Project::saveToFile(const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << serialize();
    return true;
}

bool Project::discoverSourceAssets() {
    return discoverSourceAssets(m_config.sourceDirectory);
}

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
        const std::string ext = it->path().extension().string();
        std::string normalized = ext;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (normalized == ".swf" || normalized == ".ipa") candidates.push_back(it->path());
    }

    std::sort(candidates.begin(), candidates.end());
    for (const auto& candidate : candidates) {
        std::string normalized = candidate.extension().string();
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (normalized == ".swf" && m_config.sourceSwf.empty()) {
            m_config.sourceSwf = candidate.string();
        } else if (normalized == ".ipa" && m_config.sourceIpa.empty()) {
            m_config.sourceIpa = candidate.string();
        }
    }

    return !m_config.sourceSwf.empty();
}

bool Project::hasValidSwf() const {
    if (m_config.sourceSwf.empty()) return false;
    std::string path = m_config.sourceSwf;
    std::transform(path.begin(), path.end(), path.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return path.size() >= 4 && path.compare(path.size() - 4, 4, ".swf") == 0;
}

bool Project::hasValidIpa() const {
    if (m_config.sourceIpa.empty()) return false;
    std::string path = m_config.sourceIpa;
    std::transform(path.begin(), path.end(), path.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return path.size() >= 4 && path.compare(path.size() - 4, 4, ".ipa") == 0;
}

} // namespace btd4
