#include "Project.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace btd4 {

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

bool Project::hasValidSwf() const {
    return !m_config.sourceSwf.empty() && (m_config.sourceSwf.find(".swf") != std::string::npos);
}

bool Project::hasValidIpa() const {
    return !m_config.sourceIpa.empty() && (m_config.sourceIpa.find(".ipa") != std::string::npos);
}

} // namespace btd4
