#pragma once

#include "../../engine/game/TowerData.hpp"
#include <imgui.h>
#include <string>
#include <vector>

namespace btd4 {

class TowerEditor {
public:
    void initialize();
    void render(const std::string& projectRoot);

private:
    TowerSet m_towers;
    int m_selected{-1};
    std::string m_status;
    bool m_statusGood{true};
    std::vector<std::string> m_files;
    int m_selectedFile{-1};

    void loadDefaults(const std::string& projectRoot);
    void refreshFiles(const std::string& projectRoot);
    bool loadFile(const std::string& path);
    bool saveFile(const std::string& projectRoot);
    void normalizeSelection();
};

} // namespace btd4
