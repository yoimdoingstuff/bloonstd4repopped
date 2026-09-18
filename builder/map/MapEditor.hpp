#pragma once

#include "../../engine/map/Map.hpp"
#include <string>
#include <vector>

namespace btd4 {

class MapEditor {
public:
    MapEditor();

    void initialize();
    void render(const std::string& projectRoot);

private:
    enum class Tool {
        AddPath,
        BuildableRegion,
        BlockedRegion
    };

    Map m_map{"New Map"};
    Tool m_tool{Tool::AddPath};
    int m_selectedPath{-1};
    int m_selectedWaypoint{-1};
    int m_selectedMapFile{-1};
    bool m_draggingRegion{false};
    Point2D m_dragStart{};
    Point2D m_dragCurrent{};
    std::vector<std::string> m_mapFiles;
    std::string m_lastSavedPath;
    std::string m_status;
    bool m_statusGood{true};
    char m_nameBuffer[128]{"New Map"};

    void refreshMapFiles(const std::string& projectRoot);
    void newMap();
    bool loadMapFile(const std::string& path);
    bool saveMapFile(const std::string& projectRoot);
    bool validate(std::string& error) const;
    std::string safeFileName() const;

    Point2D canvasToMap(const struct ImVec2& screen, const struct ImVec2& origin,
                        const struct ImVec2& size) const;
    struct ImVec2 mapToCanvas(const Point2D& point, const struct ImVec2& origin,
                              const struct ImVec2& size) const;
    bool pointNear(const Point2D& a, const Point2D& b, float radius) const;
};

} // namespace btd4
