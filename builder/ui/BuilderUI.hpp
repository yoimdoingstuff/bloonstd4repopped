#pragma once

#include "../project/Project.hpp"
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

    Project& project() { return m_project; }

private:
    Project m_project;
    std::vector<std::string> m_logs;
    bool m_autoScrollLogs{true};

    // Buffer for text inputs
    char m_swfPathBuffer[512]{""};
    char m_ipaPathBuffer[512]{""};

    void renderSourceFilesSection();
    void renderFeaturesSection();
    void renderPlatformSection();
    void renderActionButtons();
    void renderLogsSection();

    void triggerBuild();
};

} // namespace btd4
