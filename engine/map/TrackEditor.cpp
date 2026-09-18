#include "TrackEditor.hpp"

#include "MapLoader.hpp"
#include "../../platform/common/NativeFileSystem.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <string>
#include <utility>

namespace btd4 {
namespace fs = std::filesystem;

namespace {
constexpr float kGridWidth = 24.0f * 16.0f;
constexpr float kGridHeight = 14.0f * 16.0f;

Color styleGround(int style) {
    switch (style) {
        case 1: return {222, 195, 130, 255}; // desert
        case 2: return {150, 180, 205, 255}; // stone/grey
        case 3: return {205, 230, 245, 255}; // snow
        default: return {75, 150, 75, 255};  // grass
    }
}

Color styleRoad(int style) {
    switch (style) {
        case 1: return {182, 145, 92, 255};
        case 2: return {120, 125, 130, 255};
        case 3: return {220, 235, 245, 255};
        default: return {186, 156, 116, 255};
    }
}

Color styleRoadEdge(int style) {
    switch (style) {
        case 1: return {130, 98, 55, 255};
        case 2: return {75, 80, 85, 255};
        case 3: return {170, 195, 215, 255};
        default: return {125, 98, 70, 255};
    }
}

Color styleGrid(int style) {
    return style == 3 ? Color{120, 145, 160, 90} : Color{255, 255, 255, 45};
}
}

void TrackEditor::open(const Map& sourceMap) {
    m_open = true;
    m_applyRequested = false;
    m_map = sourceMap;
    m_tool = Tool::Road;
    m_trackStyle = 0;
    m_status = "Track Editor: build a straight-line track on the grid.";
    m_statusGood = true;
    rebuildCellsFromMap();
}

void TrackEditor::close() {
    m_open = false;
    m_applyRequested = false;
}

bool TrackEditor::consumeApplyRequest(Map& outputMap) {
    if (!m_applyRequested) return false;
    outputMap = m_map;
    m_applyRequested = false;
    m_open = false;
    return true;
}

bool TrackEditor::isInsideGrid(float x, float y) const {
    return x >= kGridOriginX && x < kGridOriginX + kGridWidth &&
           y >= kGridOriginY && y < kGridOriginY + kGridHeight;
}

int TrackEditor::cellFromPointer(float x, float y) const {
    if (!isInsideGrid(x, y)) return -1;
    const int gx = static_cast<int>((x - kGridOriginX) / kCellSize);
    const int gy = static_cast<int>((y - kGridOriginY) / kCellSize);
    if (gx < 0 || gx >= kColumns || gy < 0 || gy >= kRows) return -1;
    return gy * kColumns + gx;
}

float TrackEditor::cellCenterX(int cell) const {
    return kGridOriginX + static_cast<float>(cellX(cell)) * kCellSize + kCellSize * 0.5f;
}

float TrackEditor::cellCenterY(int cell) const {
    return kGridOriginY + static_cast<float>(cellY(cell)) * kCellSize + kCellSize * 0.5f;
}

bool TrackEditor::isBoundaryCell(int cell) const {
    const int x = cellX(cell);
    const int y = cellY(cell);
    return x == 0 || x == kColumns - 1 || y == 0 || y == kRows - 1;
}

bool TrackEditor::isAdjacent(int a, int b) const {
    const int dx = std::abs(cellX(a) - cellX(b));
    const int dy = std::abs(cellY(a) - cellY(b));
    return (dx + dy) == 1;
}

int TrackEditor::findCell(int x, int y) const {
    if (x < 0 || x >= kColumns || y < 0 || y >= kRows) return -1;
    return y * kColumns + x;
}

void TrackEditor::rebuildMapFromCells() {
    Map rebuilt("Track Editor Map");
    rebuilt.paths().push_back(Path{});

    auto& points = rebuilt.paths().front().waypoints();
    for (const int cell : m_pathCells) {
        points.push_back({cellCenterX(cell), cellCenterY(cell)});
    }
    rebuilt.paths().front().recalculate();

    // BTD4-style track editor maps are otherwise open terrain.
    rebuilt.addBuildableRegion({0.0f, 0.0f, 480.0f, 272.0f});
    m_map = std::move(rebuilt);
}

void TrackEditor::rebuildCellsFromMap() {
    m_pathCells.clear();
    m_hasStart = false;
    m_hasFinish = false;

    if (m_map.paths().empty()) {
        return;
    }

    const auto& points = m_map.paths().front().waypoints();
    for (const auto& point : points) {
        const int x = static_cast<int>(std::lround((point.x - kGridOriginX - kCellSize * 0.5f) / kCellSize));
        const int y = static_cast<int>(std::lround((point.y - kGridOriginY - kCellSize * 0.5f) / kCellSize));
        const int cell = findCell(x, y);
        if (cell >= 0 && (m_pathCells.empty() || m_pathCells.back() != cell)) {
            m_pathCells.push_back(cell);
        }
    }

    if (!m_pathCells.empty()) {
        m_hasStart = true;
        m_hasFinish = m_pathCells.size() > 1;
    }
}

void TrackEditor::setStatus(const std::string& text, bool good) {
    m_status = text;
    m_statusGood = good;
}

bool TrackEditor::setStartCell(int cell) {
    if (cell < 0 || !isBoundaryCell(cell)) {
        setStatus("Start must touch the edge of the map.", false);
        return false;
    }

    if (!m_pathCells.empty()) {
        const int oldStart = m_pathCells.front();
        if (cell == oldStart) {
            m_hasStart = true;
            return true;
        }
        if (m_pathCells.size() > 1 && !isAdjacent(cell, m_pathCells[1])) {
            setStatus("Start must connect to the first road cell.", false);
            return false;
        }
        m_pathCells.front() = cell;
    } else {
        m_pathCells.push_back(cell);
    }

    m_hasStart = true;
    rebuildMapFromCells();
    setStatus("Start placed. Select Road and extend the track.", true);
    return true;
}

bool TrackEditor::setFinishCell(int cell) {
    if (cell < 0 || !isBoundaryCell(cell)) {
        setStatus("Finish must touch the edge of the map.", false);
        return false;
    }
    if (m_pathCells.empty()) {
        setStatus("Place a Start first.", false);
        return false;
    }

    if (m_pathCells.size() > 1) {
        const int previous = m_pathCells[m_pathCells.size() - 2];
        if (cell != m_pathCells.back() && !isAdjacent(cell, previous)) {
            setStatus("Finish must connect to the current end of the track.", false);
            return false;
        }
    }
    if (m_pathCells.back() != cell) {
        m_pathCells.push_back(cell);
    }
    m_hasFinish = true;
    rebuildMapFromCells();
    setStatus("Finish placed. The track is ready to test when valid.", true);
    return true;
}

bool TrackEditor::appendRoadCell(int cell) {
    if (cell < 0) return false;
    if (m_pathCells.empty()) {
        if (!setStartCell(cell)) return false;
        m_tool = Tool::Road;
        return true;
    }

    if (cell == m_pathCells.back()) return true;
    if (!isAdjacent(cell, m_pathCells.back())) {
        setStatus("Track pieces must connect to the last cell.", false);
        return false;
    }

    // A track editor is intentionally linear. Removing the last tile is the
    // way to back up, matching the simple straight-segment workflow.
    if (std::find(m_pathCells.begin(), m_pathCells.end(), cell) != m_pathCells.end()) {
        setStatus("The track cannot loop back over an existing tile.", false);
        return false;
    }

    m_pathCells.push_back(cell);
    m_hasFinish = false;
    rebuildMapFromCells();
    setStatus("Road tile added.", true);
    return true;
}

bool TrackEditor::eraseCell(int cell) {
    if (m_pathCells.empty()) {
        setStatus("Nothing to erase.", false);
        return false;
    }

    const auto it = std::find(m_pathCells.begin(), m_pathCells.end(), cell);
    if (it == m_pathCells.end()) {
        setStatus("Select a road tile or endpoint to erase.", false);
        return false;
    }

    const size_t index = static_cast<size_t>(std::distance(m_pathCells.begin(), it));
    if (index + 1 != m_pathCells.size() && index != 0) {
        setStatus("Erase from the ends of the track to preserve a valid straight path.", false);
        return false;
    }

    if (index == 0 && m_pathCells.size() > 1) {
        m_pathCells.erase(m_pathCells.begin());
        m_hasStart = false;
    } else {
        m_pathCells.pop_back();
        m_hasFinish = false;
    }

    rebuildMapFromCells();
    setStatus("Road tile erased.", true);
    return true;
}

bool TrackEditor::save() {
    if (m_pathCells.size() < 2 || !m_hasStart || !m_hasFinish) {
        setStatus("Save requires a complete track with Start and Finish.", false);
        return false;
    }

    if (!isBoundaryCell(m_pathCells.front()) || !isBoundaryCell(m_pathCells.back())) {
        setStatus("Start and Finish must touch the edge of the map.", false);
        return false;
    }

    rebuildMapFromCells();
    m_map.setName("Track Editor Map");
    std::string error;
    const fs::path dir = fs::path("maps");
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        setStatus("Could not create maps directory: " + ec.message(), false);
        return false;
    }
    if (!btd4::saveMap((dir / "track_editor.json").string(), m_map, error)) {
        setStatus("Save failed: " + error, false);
        return false;
    }

    setStatus("Saved maps/track_editor.json.", true);
    return true;
}

bool TrackEditor::load() {
    NativeFileSystem files;
    Map loaded;
    std::string error;
    if (!loadMap(files, "maps/track_editor.json", loaded, error)) {
        setStatus("Load failed: " + error, false);
        return false;
    }
    m_map = std::move(loaded);
    rebuildCellsFromMap();
    setStatus("Loaded maps/track_editor.json.", true);
    return true;
}

void TrackEditor::update(const IInput& input, const PointerState& pointer) {
    if (!m_open) return;

    if (input.isActionJustPressed(InputAction::Cancel)) {
        close();
        return;
    }

    int toolSelection = -1;
    if (input.isActionJustPressed(InputAction::SelectTower1)) toolSelection = 0;
    else if (input.isActionJustPressed(InputAction::SelectTower2)) toolSelection = 1;
    else if (input.isActionJustPressed(InputAction::SelectTower3)) toolSelection = 2;
    else if (input.isActionJustPressed(InputAction::SelectTower4)) toolSelection = 3;

    if (toolSelection == 0) {
        m_tool = Tool::Road;
        setStatus("Tool: Road", true);
    } else if (toolSelection == 1) {
        m_tool = Tool::Start;
        setStatus("Tool: Start", true);
    } else if (toolSelection == 2) {
        m_tool = Tool::Finish;
        setStatus("Tool: Finish", true);
    } else if (toolSelection == 3) {
        m_tool = Tool::Erase;
        setStatus("Tool: Erase", true);
    }

    if (input.isActionJustPressed(InputAction::NextTarget)) {
        m_trackStyle = (m_trackStyle + 1) % 4;
        setStatus("Track style changed.", true);
    }
    if (input.isActionJustPressed(InputAction::PrevTarget)) {
        m_trackStyle = (m_trackStyle + 3) % 4;
        setStatus("Track style changed.", true);
    }

    if (!input.isActionJustPressed(InputAction::Confirm)) return;

    // Toolbar buttons live on the right of the grid.
    if (pointer.logicalX >= 356.0f && pointer.logicalX <= 474.0f) {
        if (pointer.logicalY >= 24.0f && pointer.logicalY < 48.0f) {
            m_tool = Tool::Road; setStatus("Tool: Road", true); return;
        }
        if (pointer.logicalY >= 48.0f && pointer.logicalY < 72.0f) {
            m_tool = Tool::Start; setStatus("Tool: Start", true); return;
        }
        if (pointer.logicalY >= 72.0f && pointer.logicalY < 96.0f) {
            m_tool = Tool::Finish; setStatus("Tool: Finish", true); return;
        }
        if (pointer.logicalY >= 96.0f && pointer.logicalY < 120.0f) {
            m_tool = Tool::Erase; setStatus("Tool: Erase", true); return;
        }
        if (pointer.logicalY >= 126.0f && pointer.logicalY < 150.0f) {
            save(); return;
        }
        if (pointer.logicalY >= 150.0f && pointer.logicalY < 174.0f) {
            load(); return;
        }
        if (pointer.logicalY >= 174.0f && pointer.logicalY < 198.0f) {
            rebuildMapFromCells();
            m_applyRequested = true;
            setStatus("Play test requested.", true);
            return;
        }
        if (pointer.logicalY >= 198.0f && pointer.logicalY < 222.0f) {
            close(); return;
        }
        if (pointer.logicalY >= 226.0f && pointer.logicalY < 250.0f) {
            if (pointer.logicalX < 386.0f) m_trackStyle = 0;
            else if (pointer.logicalX < 416.0f) m_trackStyle = 1;
            else if (pointer.logicalX < 446.0f) m_trackStyle = 2;
            else m_trackStyle = 3;
            setStatus("Track style changed.", true);
            return;
        }
    }

    const int cell = cellFromPointer(pointer.logicalX, pointer.logicalY);
    if (cell < 0) return;

    switch (m_tool) {
        case Tool::Road: appendRoadCell(cell); break;
        case Tool::Start: setStartCell(cell); break;
        case Tool::Finish: setFinishCell(cell); break;
        case Tool::Erase: eraseCell(cell); break;
    }
}

void TrackEditor::render(IRenderer& renderer) const {
    if (!m_open) return;

    renderer.clear(styleGround(m_trackStyle));

    // Frame and title.
    renderer.drawRect(0.0f, 0.0f, 480.0f, 272.0f, Color{25, 35, 30, 255}, false);
    renderer.drawRect(4.0f, 4.0f, 472.0f, 18.0f, {20, 20, 20, 235}, true);
    renderer.drawText("BTD4 TRACK EDITOR", 10.0f, 9.0f, 1.0f, Color::white());

    // Grid.
    renderer.drawRect(kGridOriginX, kGridOriginY, kGridWidth, kGridHeight, styleGround(m_trackStyle), true);
    for (int x = 0; x <= kColumns; ++x) {
        const float px = kGridOriginX + x * kCellSize;
        renderer.drawLine(px, kGridOriginY, px, kGridOriginY + kGridHeight, styleGrid(m_trackStyle));
    }
    for (int y = 0; y <= kRows; ++y) {
        const float py = kGridOriginY + y * kCellSize;
        renderer.drawLine(kGridOriginX, py, kGridOriginX + kGridWidth, py, styleGrid(m_trackStyle));
    }

    for (size_t i = 0; i < m_pathCells.size(); ++i) {
        const int cell = m_pathCells[i];
        const float x = kGridOriginX + cellX(cell) * kCellSize;
        const float y = kGridOriginY + cellY(cell) * kCellSize;
        renderer.drawRect(x + 1.0f, y + 1.0f, kCellSize - 2.0f, kCellSize - 2.0f, styleRoad(m_trackStyle), true);
        renderer.drawRect(x + 1.0f, y + 1.0f, kCellSize - 2.0f, kCellSize - 2.0f, styleRoadEdge(m_trackStyle), false);
    }

    if (m_hasStart && !m_pathCells.empty()) {
        const int cell = m_pathCells.front();
        renderer.drawRect(kGridOriginX + cellX(cell) * kCellSize + 3.0f,
                          kGridOriginY + cellY(cell) * kCellSize + 3.0f,
                          kCellSize - 6.0f, kCellSize - 6.0f,
                          {50, 210, 90, 255}, true);
        renderer.drawText("S", cellCenterX(cell) - 3.0f, cellCenterY(cell) - 5.0f, 1.0f, Color::black());
    }
    if (m_hasFinish && !m_pathCells.empty()) {
        const int cell = m_pathCells.back();
        renderer.drawRect(kGridOriginX + cellX(cell) * kCellSize + 3.0f,
                          kGridOriginY + cellY(cell) * kCellSize + 3.0f,
                          kCellSize - 6.0f, kCellSize - 6.0f,
                          {230, 80, 70, 255}, true);
        renderer.drawText("E", cellCenterX(cell) - 3.0f, cellCenterY(cell) - 5.0f, 1.0f, Color::white());
    }

    // Minimal right-hand toolbar, deliberately compact and Flash-era rather
    // than a modern tabbed editor.
    renderer.drawRect(350.0f, 24.0f, 126.0f, 226.0f, {15, 20, 18, 235}, true);
    renderer.drawRect(350.0f, 24.0f, 126.0f, 226.0f, Color::white(), false);

    auto button = [&renderer](float y, const char* text, bool selected) {
        renderer.drawRect(356.0f, y, 114.0f, 22.0f,
                          selected ? Color{55, 95, 145, 255} : Color{35, 40, 38, 255}, true);
        renderer.drawRect(356.0f, y, 114.0f, 22.0f,
                          selected ? Color::cyan() : Color{95, 100, 95, 255}, false);
        renderer.drawText(text, 362.0f, y + 7.0f, 1.0f, Color::white());
    };

    button(26.0f, "1 ROAD", m_tool == Tool::Road);
    button(50.0f, "2 START", m_tool == Tool::Start);
    button(74.0f, "3 FINISH", m_tool == Tool::Finish);
    button(98.0f, "4 ERASE", m_tool == Tool::Erase);

    button(126.0f, "SAVE TRACK", false);
    button(150.0f, "LOAD TRACK", false);
    button(174.0f, "PLAY TEST", false);
    button(198.0f, "DONE", false);

    renderer.drawText("STYLE", 356.0f, 230.0f, 1.0f, Color::cyan());
    const char* styles[] = {"G", "D", "S", "W"};
    for (int i = 0; i < 4; ++i) {
        const float x = 386.0f + i * 30.0f;
        renderer.drawRect(x, 226.0f, 26.0f, 20.0f,
                          i == m_trackStyle ? Color{55, 95, 145, 255} : Color{35, 40, 38, 255}, true);
        renderer.drawText(styles[i], x + 9.0f, 232.0f, 1.0f, Color::white());
    }

    if (!m_status.empty()) {
        renderer.drawRect(8.0f, 252.0f, 338.0f, 14.0f, {0, 0, 0, 190}, true);
        renderer.drawText(m_status, 12.0f, 256.0f, 1.0f,
                          m_statusGood ? Color::green() : Color::red());
    }
}

} // namespace btd4
