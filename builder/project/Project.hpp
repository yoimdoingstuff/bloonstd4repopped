#pragma once

#include <string>
#include <vector>

namespace btd4 {

struct ProjectConfig {
    int version{2};
    std::string projectName{"BTD4 Project"};
    std::string sourceDirectory{"assets"};
    std::string sourceSwf;
    std::string sourceIpa;
    std::string sourceExpansionSwf;
    std::string sourceHdIpa;
    std::string sourceMobileIpa;
    std::string gameEdition{"BTD4 Flash"};
    bool enableMobileContent{false};
    std::string targetPlatform{"Linux"};
    std::string buildConfiguration{"Release"};
};

class Project {
public:
    Project();

    const ProjectConfig& config() const { return m_config; }
    ProjectConfig& config() { return m_config; }

    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath) const;

    std::string serialize() const;
    bool deserialize(const std::string& json);

    bool discoverSourceAssets();
    bool discoverSourceAssets(const std::string& directory);
    bool addSourceFile(const std::string& filepath);

    bool hasValidSwf() const;
    bool hasValidIpa() const;

private:
    ProjectConfig m_config;
};

} // namespace btd4
