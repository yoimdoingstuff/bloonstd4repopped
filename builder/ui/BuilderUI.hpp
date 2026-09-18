#pragma once

#include "../project/Project.hpp"
#include "../map/MapEditor.hpp"
#include "../round/RoundEditor.hpp"
#include "../tower/TowerEditor.hpp"
#include "../upgrade/UpgradeEditor.hpp"
#include "../../platform/common/PlatformRegistry.hpp"
#include <vector>
#include <string>

namespace btd4 {

class BuilderUI {
public:
    BuilderUI();

    void initialize();
    void render();

    void appendLog(const std::string& line);
    void clearLogs();
    void addSourceFile(const std::string& path);

    Project& project() { return m_project; }

private:
    Project m_project;
    MapEditor m_mapEditor;
    RoundEditor m_roundEditor;
    TowerEditor m_towerEditor;
    UpgradeEditor m_upgradeEditor;
    std::vector<std::string> m_logs;
    bool m_autoScrollLogs{true};
    std::string m_lastSuccessfulImportKey;

    char m_sourceDirectoryBuffer[512]{"assets"};
    char m_swfPathBuffer[512]{""};
    char m_ipaPathBuffer[512]{""};
    char m_expansionSwfPathBuffer[512]{""};
    char m_hdIpaPathBuffer[512]{""};
    char m_mobileIpaPathBuffer[512]{""};

    void renderSourceFilesSection();
    void renderFeaturesSection();
    void renderPlatformSection();
    void renderActionButtons();
    void renderLogsSection();

    void discoverAssets(bool forceRescan = false);
    bool validateProject();
    void triggerBuild();
    void previewBuild();
    bool triggerImport();
};

} // namespace btd4
