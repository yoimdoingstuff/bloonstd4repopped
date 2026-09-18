#pragma once

#include "../../engine/game/Rounds.hpp"
#include "../../engine/map/Map.hpp"
#include <imgui.h>
#include <string>
#include <vector>

namespace btd4 {

class RoundEditor {
public:
    RoundEditor();

    void initialize();
    void render(const std::string& projectRoot, const Map& map);

private:
    RoundSet m_rounds;
    int m_selectedRound{0};
    int m_selectedGroup{0};
    int m_selectedFile{-1};
    std::vector<std::string> m_roundFiles;
    std::string m_status;
    bool m_statusGood{true};

    void ensureDefaultLoaded(const std::string& projectRoot, const Map& map);
    void refreshFiles(const std::string& projectRoot);
    void newRound();
    bool loadFile(const std::string& path, const Map& map);
    bool saveFile(const std::string& projectRoot, const Map& map);
    void normalizeSelection();
    static const char* bloonName(BloonType type);
    static BloonType bloonTypeFromIndex(int index);
};

} // namespace btd4
