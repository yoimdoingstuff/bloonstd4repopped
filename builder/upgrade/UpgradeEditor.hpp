#pragma once

#include "../../engine/game/Upgrade.hpp"
#include "../../engine/game/TowerData.hpp"
#include <imgui.h>
#include <string>
#include <vector>

namespace btd4 {

class UpgradeEditor {
public:
    void initialize();
    void render(const std::string& projectRoot);

private:
    UpgradeSet m_upgrades;
    int m_selected{-1};
    int m_selectedFile{-1};
    std::vector<std::string> m_files;
    std::string m_status;
    bool m_statusGood{true};

    void loadDefaults(const std::string& projectRoot);
    void refreshFiles(const std::string& projectRoot);
    bool loadFile(const std::string& path);
    bool saveFile(const std::string& projectRoot);
    void normalizeSelection();
};

} // namespace btd4
