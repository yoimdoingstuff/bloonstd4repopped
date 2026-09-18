#include "MapEditor.hpp"

#include "../../engine/core/IFileSystem.hpp"
#include "../../engine/map/MapLoader.hpp"
#include "../../platform/common/NativeFileSystem.hpp"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace btd4 {
namespace fs = std::filesystem;

MapEditor::MapEditor() = default;

void MapEditor::initialize() {
    newMap();
}

void MapEditor::newMap() {
    m_map = Map("New Map");
    m_tool = Tool::AddPath;
    m_selectedPath = -1;
    m_selectedWaypoint = -1;
    m_draggingRegion = false;
    m_lastSavedPath.clear();
    m_status = "New map created.";
    m_statusGood = true;
    std::strncpy(m_nameBuffer, "New Map", sizeof(m_nameBuffer) - 1);
    m_nameBuffer[sizeof(m_nameBuffer) - 1] = '\0';
}

std::string MapEditor::safeFileName() const {
    std::string result = m_map.name();
    if (result.empty()) result = "map";
    for (char& c : result) {
        const bool ok = (std::isalnum(static_cast<unsigned char>(c)) != 0) ||
                        c == '_' || c == '-';
        if (!ok) c = '_';
    }
    return result + ".json";
}

void MapEditor::refreshMapFiles(const std::string& projectRoot) {
    m_mapFiles.clear();
    const fs::path dir = fs::path(projectRoot) / "maps";
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return;

    for (fs::directory_iterator it(dir, ec), end; it != end && !ec; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        if (it->path().extension() == ".json") {
            m_mapFiles.push_back(it->path().string());
        }
    }
    std::sort(m_mapFiles.begin(), m_mapFiles.end());
    if (m_selectedMapFile >= static_cast<int>(m_mapFiles.size())) {
        m_selectedMapFile = m_mapFiles.empty() ? -1 : 0;
    }
}

bool MapEditor::loadMapFile(const std::string& path) {
    NativeFileSystem files;
    Map loaded;
    std::string error;
    if (!loadMap(files, path, loaded, error)) {
        m_status = "Load failed: " + error;
        m_statusGood = false;
        return false;
    }

    m_map = std::move(loaded);
    std::strncpy(m_nameBuffer, m_map.name().c_str(), sizeof(m_nameBuffer) - 1);
    m_nameBuffer[sizeof(m_nameBuffer) - 1] = '\0';
    m_selectedPath = m_map.paths().empty() ? -1 : 0;
    m_selectedWaypoint = -1;
    m_lastSavedPath = path;
    m_status = "Loaded " + path;
    m_statusGood = true;
    return true;
}

bool MapEditor::validate(std::string& error) const {
    if (m_map.name().empty()) {
        error = "Map name cannot be empty.";
        return false;
    }
    if (m_map.paths().empty()) {
        error = "Add at least one path.";
        return false;
    }
    for (size_t i = 0; i < m_map.paths().size(); ++i) {
        const auto& path = m_map.paths()[i];
        if (path.waypointCount() < 2) {
            error = "Path " + std::to_string(i + 1) + " needs at least two waypoints.";
            return false;
        }
        if (path.totalLength() <= 0.0f) {
            error = "Path " + std::to_string(i + 1) + " has zero length.";
            return false;
        }
    }
    return m_map.validate();
}

bool MapEditor::saveMapFile(const std::string& projectRoot) {
    m_map.setName(m_nameBuffer);
    std::string error;
    if (!validate(error)) {
        m_status = "Validation failed: " + error;
        m_statusGood = false;
        return false;
    }

    const fs::path dir = fs::path(projectRoot) / "maps";
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        m_status = "Could not create maps directory: " + ec.message();
        m_statusGood = false;
        return false;
    }

    const fs::path target = dir / safeFileName();
    if (!saveMap(target.string(), m_map, error)) {
        m_status = "Save failed: " + error;
        m_statusGood = false;
        return false;
    }

    m_lastSavedPath = target.string();
    refreshMapFiles(projectRoot);
    m_status = "Saved " + target.string();
    m_statusGood = true;
    return true;
}

Point2D MapEditor::canvasToMap(const ImVec2& screen, const ImVec2& origin,
                               const ImVec2& size) const {
    const float scale = std::min(size.x / 480.0f, size.y / 272.0f);
    const float drawW = 480.0f * scale;
    const float drawH = 272.0f * scale;
    const float ox = origin.x + (size.x - drawW) * 0.5f;
    const float oy = origin.y + (size.y - drawH) * 0.5f;
    return {(screen.x - ox) / scale, (screen.y - oy) / scale};
}

ImVec2 MapEditor::mapToCanvas(const Point2D& point, const ImVec2& origin,
                              const ImVec2& size) const {
    const float scale = std::min(size.x / 480.0f, size.y / 272.0f);
    const float drawW = 480.0f * scale;
    const float drawH = 272.0f * scale;
    const float ox = origin.x + (size.x - drawW) * 0.5f;
    const float oy = origin.y + (size.y - drawH) * 0.5f;
    return {ox + point.x * scale, oy + point.y * scale};
}

bool MapEditor::pointNear(const Point2D& a, const Point2D& b, float radius) const {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy <= radius * radius;
}

void MapEditor::render(const std::string& projectRoot) {
    if (!ImGui::CollapsingHeader("Map Editor", ImGuiTreeNodeFlags_DefaultOpen)) return;

    if (ImGui::InputText("Map Name", m_nameBuffer, sizeof(m_nameBuffer))) {
        m_map.setName(m_nameBuffer);
    }

    if (ImGui::Button("New Map")) newMap();
    ImGui::SameLine();
    if (ImGui::Button("Save Map")) saveMapFile(projectRoot);

    ImGui::SameLine();
    if (ImGui::Button("Validate")) {
        std::string error;
        if (validate(error)) {
            m_status = "Map is valid.";
            m_statusGood = true;
        } else {
            m_status = error;
            m_statusGood = false;
        }
    }

    refreshMapFiles(projectRoot);
    if (!m_mapFiles.empty()) {
        std::vector<const char*> labels;
        labels.reserve(m_mapFiles.size());
        for (const auto& file : m_mapFiles) labels.push_back(file.c_str());
        if (m_selectedMapFile < 0) m_selectedMapFile = 0;
        if (ImGui::Combo("Saved Maps", &m_selectedMapFile, labels.data(), static_cast<int>(labels.size()))) {
            if (m_selectedMapFile >= 0 && m_selectedMapFile < static_cast<int>(m_mapFiles.size())) {
                loadMapFile(m_mapFiles[m_selectedMapFile]);
            }
        }
    }

    if (ImGui::RadioButton("Path", m_tool == Tool::AddPath)) m_tool = Tool::AddPath;
    ImGui::SameLine();
    if (ImGui::RadioButton("Buildable", m_tool == Tool::BuildableRegion)) m_tool = Tool::BuildableRegion;
    ImGui::SameLine();
    if (ImGui::RadioButton("Blocked", m_tool == Tool::BlockedRegion)) m_tool = Tool::BlockedRegion;

    if (m_tool == Tool::AddPath) {
        if (ImGui::Button("New Path")) {
            m_map.paths().push_back(Path{});
            m_selectedPath = static_cast<int>(m_map.paths().size()) - 1;
            m_selectedWaypoint = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Path") && m_selectedPath >= 0 &&
            m_selectedPath < static_cast<int>(m_map.paths().size())) {
            m_map.paths().erase(m_map.paths().begin() + m_selectedPath);
            m_selectedPath = std::clamp(m_selectedPath - 1, -1, static_cast<int>(m_map.paths().size()) - 1);
            m_selectedWaypoint = -1;
        }

        if (!m_map.paths().empty()) {
            std::vector<std::string> names;
            names.reserve(m_map.paths().size());
            for (size_t i = 0; i < m_map.paths().size(); ++i) {
                names.push_back("Path " + std::to_string(i + 1));
            }
            std::vector<const char*> labels;
            labels.reserve(names.size());
            for (const auto& name : names) labels.push_back(name.c_str());
            if (m_selectedPath < 0) m_selectedPath = 0;
            ImGui::Combo("Active Path", &m_selectedPath, labels.data(), static_cast<int>(labels.size()));
            if (m_selectedPath >= 0 && m_selectedPath < static_cast<int>(m_map.paths().size())) {
                ImGui::Text("Waypoints: %d", static_cast<int>(m_map.paths()[m_selectedPath].waypointCount()));
                if (ImGui::Button("Delete Last Waypoint") &&
                    !m_map.paths()[m_selectedPath].waypoints().empty()) {
                    auto& points = m_map.paths()[m_selectedPath].waypoints();
                    points.pop_back();
                    m_map.paths()[m_selectedPath].recalculate();
                }
            }
        }
    }

    ImGui::TextDisabled("Click the canvas to add path points or place region corners. Right-click removes the nearest waypoint.");
    ImGui::BeginChild("MapEditorCanvas", ImVec2(0, 360), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    const float scale = std::min(canvasSize.x / 480.0f, canvasSize.y / 272.0f);
    const float drawW = 480.0f * scale;
    const float drawH = 272.0f * scale;
    const ImVec2 mapOrigin(canvasOrigin.x + (canvasSize.x - drawW) * 0.5f,
                           canvasOrigin.y + (canvasSize.y - drawH) * 0.5f);
    const ImVec2 mapEnd(mapOrigin.x + drawW, mapOrigin.y + drawH);

    draw->AddRectFilled(mapOrigin, mapEnd, IM_COL32(28, 120, 28, 255));
    for (int x = 0; x <= 480; x += 40) {
        const ImVec2 a = mapToCanvas({static_cast<float>(x), 0.0f}, canvasOrigin, canvasSize);
        const ImVec2 b = mapToCanvas({static_cast<float>(x), 272.0f}, canvasOrigin, canvasSize);
        draw->AddLine(a, b, IM_COL32(255,255,255,30));
    }
    for (int y = 0; y <= 272; y += 40) {
        const ImVec2 a = mapToCanvas({0.0f, static_cast<float>(y)}, canvasOrigin, canvasSize);
        const ImVec2 b = mapToCanvas({480.0f, static_cast<float>(y)}, canvasOrigin, canvasSize);
        draw->AddLine(a, b, IM_COL32(255,255,255,30));
    }

    for (const auto& rect : m_map.buildableRegions()) {
        const ImVec2 a = mapToCanvas({rect.x, rect.y}, canvasOrigin, canvasSize);
        const ImVec2 b = mapToCanvas({rect.x + rect.w, rect.y + rect.h}, canvasOrigin, canvasSize);
        draw->AddRectFilled(a, b, IM_COL32(40, 160, 90, 45));
        draw->AddRect(a, b, IM_COL32(40, 220, 120, 200));
    }
    for (const auto& rect : m_map.blockedRegions()) {
        const ImVec2 a = mapToCanvas({rect.x, rect.y}, canvasOrigin, canvasSize);
        const ImVec2 b = mapToCanvas({rect.x + rect.w, rect.y + rect.h}, canvasOrigin, canvasSize);
        draw->AddRectFilled(a, b, IM_COL32(150, 50, 50, 80));
        draw->AddRect(a, b, IM_COL32(230, 80, 80, 220));
    }

    for (size_t p = 0; p < m_map.paths().size(); ++p) {
        const auto& path = m_map.paths()[p];
        const auto& points = path.waypoints();
        const ImU32 pathColor = static_cast<int>(p) == m_selectedPath
            ? IM_COL32(255, 230, 80, 255)
            : IM_COL32(230, 190, 120, 220);
        for (size_t i = 0; i + 1 < points.size(); ++i) {
            draw->AddLine(mapToCanvas(points[i], canvasOrigin, canvasSize),
                          mapToCanvas(points[i + 1], canvasOrigin, canvasSize),
                          pathColor, static_cast<int>(p) == m_selectedPath ? 4.0f : 2.0f);
        }
        for (size_t i = 0; i < points.size(); ++i) {
            const ImVec2 c = mapToCanvas(points[i], canvasOrigin, canvasSize);
            const bool selected = static_cast<int>(p) == m_selectedPath &&
                                  static_cast<int>(i) == m_selectedWaypoint;
            draw->AddCircleFilled(c, selected ? 6.0f : 4.0f,
                                  selected ? IM_COL32(255,255,255,255) : pathColor);
        }
    }

    ImGui::InvisibleButton("MapCanvasHitbox", canvasSize);
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const Point2D mapPoint = canvasToMap(mouse, canvasOrigin, canvasSize);
    const bool inside = hovered && mapPoint.x >= 0.0f && mapPoint.x <= 480.0f &&
                        mapPoint.y >= 0.0f && mapPoint.y <= 272.0f;

    if (inside && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && m_tool == Tool::AddPath) {
        float best = 12.0f * 12.0f;
        int bestIndex = -1;
        if (m_selectedPath >= 0 && m_selectedPath < static_cast<int>(m_map.paths().size())) {
            auto& points = m_map.paths()[m_selectedPath].waypoints();
            for (int i = 0; i < static_cast<int>(points.size()); ++i) {
                const float dx = points[i].x - mapPoint.x;
                const float dy = points[i].y - mapPoint.y;
                const float d2 = dx * dx + dy * dy;
                if (d2 < best) {
                    best = d2;
                    bestIndex = i;
                }
            }
            if (bestIndex >= 0) {
                points.erase(points.begin() + bestIndex);
                m_map.paths()[m_selectedPath].recalculate();
                m_selectedWaypoint = -1;
            }
        }
    }

    if (inside && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (m_tool == Tool::AddPath) {
            if (m_selectedPath < 0) {
                m_map.paths().push_back(Path{});
                m_selectedPath = static_cast<int>(m_map.paths().size()) - 1;
            }
            auto& points = m_map.paths()[m_selectedPath].waypoints();
            points.push_back(mapPoint);
            m_map.paths()[m_selectedPath].recalculate();
            m_selectedWaypoint = static_cast<int>(points.size()) - 1;
        } else {
            m_draggingRegion = true;
            m_dragStart = mapPoint;
            m_dragCurrent = mapPoint;
        }
    }

    if (m_draggingRegion) {
        if (inside && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            m_dragCurrent = mapPoint;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_draggingRegion = false;
            const float x = std::min(m_dragStart.x, m_dragCurrent.x);
            const float y = std::min(m_dragStart.y, m_dragCurrent.y);
            const float w = std::fabs(m_dragCurrent.x - m_dragStart.x);
            const float h = std::fabs(m_dragCurrent.y - m_dragStart.y);
            if (w >= 2.0f && h >= 2.0f) {
                if (m_tool == Tool::BuildableRegion) m_map.addBuildableRegion({x,y,w,h});
                else m_map.addBlockedRegion({x,y,w,h});
            }
        }
        const ImVec2 a = mapToCanvas(m_dragStart, canvasOrigin, canvasSize);
        const ImVec2 b = mapToCanvas(m_dragCurrent, canvasOrigin, canvasSize);
        draw->AddRect(a, b, IM_COL32(255,255,255,200), 0.0f, 0, 2.0f);
    }

    ImGui::EndChild();

    if (!m_status.empty()) {
        ImGui::TextColored(m_statusGood ? ImVec4(0.3f,1.0f,0.4f,1.0f)
                                        : ImVec4(1.0f,0.4f,0.4f,1.0f),
                           "%s", m_status.c_str());
    }
}

} // namespace btd4
