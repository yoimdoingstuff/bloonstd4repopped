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
constexpr float kGridWidth = 21.0f * 16.0f;
constexpr float kGridHeight = 14.0f * 16.0f;

Color styleGround(int style) {
    switch (style) {
        case 1: return {222, 195, 130, 255};
        case 2: return {150, 180, 205, 255};
        case 3: return {205, 230, 245, 255};
        default: return {75, 150, 75, 255};
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

const char* densityLabel(BloonDensity density) {
    switch (density) {
        case BloonDensity::Low: return "LOW";
        case BloonDensity::High: return "HIGH";
        default: return "NORMAL";
    }
}
}

void TrackEditor::open(const Map& sourceMap) {
    m_open = true;
    m_applyRequested = false;
    m_map = sourceMap;
    m_tool = Tool::Road;
    m_activePath = 0;
    m_trackSet = sourceMap.trackSet() % 4;
    m_bloonDensity = sourceMap.bloonDensity();
    m_status = "Track Editor: connect edge entrances and exits with track pieces.";
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
    Map rebuilt = m_map;
    rebuilt.paths().clear();
    rebuilt.setTrackSet(m_trackSet);
    rebuilt.setBloonDensity(m_bloonDensity);

    for (int pathIndex = 0; pathIndex < MaxPaths; ++pathIndex) {
        if (m_pathCells[pathIndex].size() < 2) continue;

        Path path;
        auto& points = path.waypoints();
        for (const int cell : m_pathCells[pathIndex]) {
            points.push_back({cellCenterX(cell), cellCenterY(cell)});
        }
        path.recalculate();
        rebuilt.addPath(path);
    }

    m_map = std::move(rebuilt);
}

void TrackEditor::rebuildCellsFromMap() {
    for (auto& cells : m_pathCells) cells.clear();
    m_hasStart.fill(false);
    m_hasFinish.fill(false);

    const int count = std::min<int>(MaxPaths, static_cast<int>(m_map.paths().size()));
    for (int pathIndex = 0; pathIndex < count; ++pathIndex) {
        const auto& source = m_map.paths()[pathIndex].waypoints();
        auto& cells = m_pathCells[pathIndex];

        auto appendCell = [&cells](int cell) {
            if (cell >= 0 && (cells.empty() || cells.back() != cell)) cells.push_back(cell);
        };
        int previousCell = -1;
        for (const auto& point : source) {
            int gx = static_cast<int>(std::lround((point.x - kGridOriginX - kCellSize * 0.5f) / kCellSize));
            int gy = static_cast<int>(std::lround((point.y - kGridOriginY - kCellSize * 0.5f) / kCellSize));
            gx = std::clamp(gx, 0, kColumns - 1);
            gy = std::clamp(gy, 0, kRows - 1);
            const int cell = findCell(gx, gy);
            if (cell < 0) continue;

            if (previousCell < 0) {
                appendCell(cell);
            } else {
                int x = cellX(previousCell);
                int y = cellY(previousCell);
                const int targetX = cellX(cell);
                const int targetY = cellY(cell);
                while (x != targetX || y != targetY) {
                    if (x != targetX) {
                        x += x < targetX ? 1 : -1;
                    } else {
                        y += y < targetY ? 1 : -1;
                    }
                    appendCell(findCell(x, y));
                }
            }
            previousCell = cell;
        }

        if (cells.size() >= 2) {
            m_hasStart[pathIndex] = isBoundaryCell(cells.front());
            m_hasFinish[pathIndex] = isBoundaryCell(cells.back());
        }
    }
}

void TrackEditor::setStatus(const std::string& text, bool good) {
    m_status = text;
    m_statusGood = good;
}

bool TrackEditor::setStartCell(int cell) {
    if (cell < 0 || !isBoundaryCell(cell)) {
        setStatus("Entrance must be placed on the edge of the track editor.", false);
        return false;
    }

    auto& cells = m_pathCells[m_activePath];
    if (!cells.empty()) {
        if (cells.front() == cell) {
            m_hasStart[m_activePath] = true;
            return true;
        }
        if (cells.size() > 1 && !isAdjacent(cell, cells[1])) {
            setStatus("Entrance must connect to the first track piece.", false);
            return false;
        }
        cells.front() = cell;
    } else {
        cells.push_back(cell);
    }

    m_hasStart[m_activePath] = true;
    if (cells.size() < 2) m_hasFinish[m_activePath] = false;
    rebuildMapFromCells();
    setStatus("Entrance placed. Select Road and extend the path.", true);
    return true;
}

bool TrackEditor::setFinishCell(int cell) {
    if (cell < 0 || !isBoundaryCell(cell)) {
        setStatus("Exit must be placed on the edge of the track editor.", false);
        return false;
    }

    auto& cells = m_pathCells[m_activePath];
    if (cells.empty()) {
        setStatus("Place an entrance first.", false);
        return false;
    }

    if (cells.size() > 1 && cells.back() != cell && !isAdjacent(cell, cells[cells.size() - 2])) {
        setStatus("Exit must connect to the last track piece.", false);
        return false;
    }
    if (cells.back() != cell) cells.push_back(cell);

    m_hasFinish[m_activePath] = true;
    rebuildMapFromCells();
    setStatus("Exit placed. This path is complete.", true);
    return true;
}

bool TrackEditor::appendRoadCell(int cell) {
    if (cell < 0) return false;

    auto& cells = m_pathCells[m_activePath];
    if (cells.empty()) {
        if (!setStartCell(cell)) return false;
        m_tool = Tool::Road;
        return true;
    }

    if (cell == cells.back()) return true;
    if (!isAdjacent(cell, cells.back())) {
        setStatus("Track pieces must connect to the current path end.", false);
        return false;
    }
    if (std::find(cells.begin(), cells.end(), cell) != cells.end()) {
        setStatus("A path cannot cross back over one of its own track pieces.", false);
        return false;
    }

    cells.push_back(cell);
    m_hasFinish[m_activePath] = false;
    rebuildMapFromCells();
    setStatus("Track piece added.", true);
    return true;
}

bool TrackEditor::eraseCell(int cell) {
    auto& cells = m_pathCells[m_activePath];
    if (cells.empty()) {
        setStatus("Nothing to erase on this path.", false);
        return false;
    }

    const auto it = std::find(cells.begin(), cells.end(), cell);
    if (it == cells.end()) {
        setStatus("Select a path end or edge marker to erase it.", false);
        return false;
    }

    const size_t index = static_cast<size_t>(std::distance(cells.begin(), it));
    if (index != 0 && index + 1 != cells.size()) {
        setStatus("Erase path pieces from the ends to keep the track connected.", false);
        return false;
    }

    if (index == 0) {
        cells.erase(cells.begin());
        m_hasStart[m_activePath] = false;
    } else {
        cells.pop_back();
        m_hasFinish[m_activePath] = false;
    }

    if (cells.empty()) {
        m_hasStart[m_activePath] = false;
        m_hasFinish[m_activePath] = false;
    }
    rebuildMapFromCells();
    setStatus("Track piece erased.", true);
    return true;
}

bool TrackEditor::validateCurrent() const {
    bool anyPath = false;
    for (int pathIndex = 0; pathIndex < MaxPaths; ++pathIndex) {
        const auto& cells = m_pathCells[pathIndex];
        if (cells.empty()) continue;

        anyPath = true;
        if (cells.size() < 2 || !m_hasStart[pathIndex] || !m_hasFinish[pathIndex]) return false;
        if (!isBoundaryCell(cells.front()) || !isBoundaryCell(cells.back())) return false;

        for (size_t i = 1; i < cells.size(); ++i) {
            if (!isAdjacent(cells[i - 1], cells[i])) return false;
        }
    }
    return anyPath;
}

bool TrackEditor::save() {
    if (!validateCurrent()) {
        setStatus("Save requires at least one complete entrance-to-exit path.", false);
        return false;
    }

    rebuildMapFromCells();
    m_map.setName("Track Editor Map");
    m_map.setTrackSet(m_trackSet);
    m_map.setBloonDensity(m_bloonDensity);

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
    m_trackSet = m_map.trackSet() % 4;
    m_bloonDensity = m_map.bloonDensity();
    m_activePath = 0;
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

    if (input.isActionJustPressed(InputAction::NextTarget)) {
        m_activePath = (m_activePath + 1) % MaxPaths;
        setStatus("Editing path " + std::to_string(m_activePath + 1) + ".", true);
    }
    if (input.isActionJustPressed(InputAction::PrevTarget)) {
        m_activePath = (m_activePath + MaxPaths - 1) % MaxPaths;
        setStatus("Editing path " + std::to_string(m_activePath + 1) + ".", true);
    }

    int toolSelection = -1;
    if (input.isActionJustPressed(InputAction::SelectTower1)) toolSelection = 0;
    else if (input.isActionJustPressed(InputAction::SelectTower2)) toolSelection = 1;
    else if (input.isActionJustPressed(InputAction::SelectTower3)) toolSelection = 2;
    else if (input.isActionJustPressed(InputAction::SelectTower4)) toolSelection = 3;

    if (toolSelection >= 0) {
        m_tool = static_cast<Tool>(toolSelection);
        setStatus(std::string("Tool: ") +
                  (m_tool == Tool::Road ? "Road" :
                   m_tool == Tool::Start ? "Entrance" :
                   m_tool == Tool::Finish ? "Exit" : "Erase"), true);
    }

    if (!input.isActionJustPressed(InputAction::Confirm)) return;

    if (pointer.logicalX >= 350.0f && pointer.logicalX <= 476.0f) {
        if (pointer.logicalY >= 24.0f && pointer.logicalY < 48.0f) {
            m_tool = Tool::Road; setStatus("Tool: Road", true); return;
        }
        if (pointer.logicalY >= 48.0f && pointer.logicalY < 72.0f) {
            m_tool = Tool::Start; setStatus("Tool: Entrance", true); return;
        }
        if (pointer.logicalY >= 72.0f && pointer.logicalY < 96.0f) {
            m_tool = Tool::Finish; setStatus("Tool: Exit", true); return;
        }
        if (pointer.logicalY >= 96.0f && pointer.logicalY < 120.0f) {
            m_tool = Tool::Erase; setStatus("Tool: Erase", true); return;
        }
        if (pointer.logicalY >= 122.0f && pointer.logicalY < 146.0f &&
            pointer.logicalX >= 386.0f) {
            const int slot = static_cast<int>((pointer.logicalX - 386.0f) / 21.0f);
            if (slot >= 0 && slot < MaxPaths) {
                m_activePath = slot;
                setStatus("Editing path " + std::to_string(slot + 1) + ".", true);
            }
            return;
        }
        if (pointer.logicalY >= 148.0f && pointer.logicalY < 172.0f) {
            save(); return;
        }
        if (pointer.logicalY >= 172.0f && pointer.logicalY < 196.0f) {
            load(); return;
        }
        if (pointer.logicalY >= 196.0f && pointer.logicalY < 220.0f) {
            if (validateCurrent()) {
                rebuildMapFromCells();
                m_map.setTrackSet(m_trackSet);
                m_map.setBloonDensity(m_bloonDensity);
                m_applyRequested = true;
                setStatus("Play test requested.", true);
            } else {
                setStatus("Play Test requires at least one complete path.", false);
            }
            return;
        }
        if (pointer.logicalY >= 220.0f && pointer.logicalY < 242.0f) {
            close(); return;
        }
        if (pointer.logicalY >= 242.0f && pointer.logicalY < 266.0f) {
            if (pointer.logicalX >= 386.0f && pointer.logicalX < 407.0f) m_trackSet = 0;
            else if (pointer.logicalX < 428.0f) m_trackSet = 1;
            else if (pointer.logicalX < 449.0f) m_trackSet = 2;
            else if (pointer.logicalX < 470.0f) m_trackSet = 3;
            else return;

            m_map.setTrackSet(m_trackSet);
            setStatus("Track set changed.", true);
            return;
        }
    }

    if (pointer.logicalX >= 250.0f && pointer.logicalX < 350.0f &&
        pointer.logicalY >= 244.0f && pointer.logicalY < 266.0f) {
        const float third = 100.0f / 3.0f;
        const int choice = std::clamp(static_cast<int>((pointer.logicalX - 250.0f) / third), 0, 2);
        m_bloonDensity = static_cast<BloonDensity>(choice);
        setStatus("Bloon density: " + std::string(densityLabel(m_bloonDensity)) + ".", true);
        return;
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

    renderer.clear(styleGround(m_trackSet));
    renderer.drawRect(0.0f, 0.0f, 480.0f, 272.0f, Color{25, 35, 30, 255}, false);
    renderer.drawRect(4.0f, 4.0f, 472.0f, 18.0f, {20, 20, 20, 235}, true);
    renderer.drawText("BTD4 TRACK EDITOR", 10.0f, 9.0f, 1.0f, Color::white());

    renderer.drawRect(kGridOriginX, kGridOriginY, kGridWidth, kGridHeight, styleGround(m_trackSet), true);
    for (int x = 0; x <= kColumns; ++x) {
        const float px = kGridOriginX + x * kCellSize;
        renderer.drawLine(px, kGridOriginY, px, kGridOriginY + kGridHeight, styleGrid(m_trackSet));
    }
    for (int y = 0; y <= kRows; ++y) {
        const float py = kGridOriginY + y * kCellSize;
        renderer.drawLine(kGridOriginX, py, kGridOriginX + kGridWidth, py, styleGrid(m_trackSet));
    }

    for (int pathIndex = 0; pathIndex < MaxPaths; ++pathIndex) {
        const auto& cells = m_pathCells[pathIndex];
        if (cells.empty()) continue;

        const bool selectedPath = pathIndex == m_activePath;
        for (size_t i = 0; i < cells.size(); ++i) {
            const int cell = cells[i];
            const float x = kGridOriginX + cellX(cell) * kCellSize;
            const float y = kGridOriginY + cellY(cell) * kCellSize;

            Color road = styleRoad(m_trackSet);
            if (!selectedPath) road = {static_cast<uint8_t>(road.r / 2), static_cast<uint8_t>(road.g / 2), static_cast<uint8_t>(road.b / 2), 255};
            renderer.drawRect(x + 1.0f, y + 1.0f, kCellSize - 2.0f, kCellSize - 2.0f, road, true);
            renderer.drawRect(x + 1.0f, y + 1.0f, kCellSize - 2.0f, kCellSize - 2.0f,
                              selectedPath ? styleRoadEdge(m_trackSet) : Color{70,70,70,255}, false);
            if (selectedPath && i + 1 < cells.size()) {
                renderer.drawLine(cellCenterX(cells[i]), cellCenterY(cells[i]),
                                  cellCenterX(cells[i + 1]), cellCenterY(cells[i + 1]),
                                  Color::white());
            }
        }

        const int start = cells.front();
        if (m_hasStart[pathIndex]) {
            renderer.drawRect(kGridOriginX + cellX(start) * kCellSize + 3.0f,
                              kGridOriginY + cellY(start) * kCellSize + 3.0f,
                              kCellSize - 6.0f, kCellSize - 6.0f,
                              {50, 210, 90, 255}, true);
            renderer.drawText("S", cellCenterX(start) - 3.0f, cellCenterY(start) - 5.0f, 1.0f, Color::black());
        }

        if (m_hasFinish[pathIndex]) {
            const int finish = cells.back();
            renderer.drawRect(kGridOriginX + cellX(finish) * kCellSize + 3.0f,
                              kGridOriginY + cellY(finish) * kCellSize + 3.0f,
                              kCellSize - 6.0f, kCellSize - 6.0f,
                              {230, 80, 70, 255}, true);
            renderer.drawText("E", cellCenterX(finish) - 3.0f, cellCenterY(finish) - 5.0f, 1.0f, Color::white());
        }
    }

    renderer.drawRect(350.0f, 24.0f, 126.0f, 242.0f, {15, 20, 18, 245}, true);
    renderer.drawRect(350.0f, 24.0f, 126.0f, 242.0f, {120, 195, 125, 240}, false);

    auto button = [&renderer](float y, const char* text, bool selected) {
        renderer.drawRect(356.0f, y, 114.0f, 22.0f,
                          selected ? Color{55, 95, 145, 255} : Color{35, 40, 38, 255}, true);
        renderer.drawRect(356.0f, y, 114.0f, 22.0f,
                          selected ? Color::cyan() : Color{95, 100, 95, 255}, false);
        renderer.drawText(text, 362.0f, y + 7.0f, 1.0f, Color::white());
    };

    button(26.0f, "1 ROAD", m_tool == Tool::Road);
    button(50.0f, "2 ENTRANCE", m_tool == Tool::Start);
    button(74.0f, "3 EXIT", m_tool == Tool::Finish);
    button(98.0f, "4 ERASE", m_tool == Tool::Erase);

    renderer.drawText("PATH", 356.0f, 128.0f, 1.0f, Color::cyan());
    for (int i = 0; i < MaxPaths; ++i) {
        const float x = 386.0f + i * 21.0f;
        renderer.drawRect(x, 122.0f, 18.0f, 20.0f,
                          i == m_activePath ? Color{55, 95, 145, 255} : Color{35, 40, 38, 255}, true);
        renderer.drawText(std::to_string(i + 1), x + 6.0f, 128.0f, 1.0f, Color::white());
    }

    button(148.0f, "SAVE TRACK", false);
    button(172.0f, "LOAD TRACK", false);
    button(196.0f, "PLAY TEST", false);
    button(220.0f, "DONE", false);

    renderer.drawText("SET", 356.0f, 232.0f, 1.0f, Color::cyan());
    for (int i = 0; i < 4; ++i) {
        const float x = 386.0f + i * 21.0f;
        renderer.drawRect(x, 226.0f, 18.0f, 20.0f,
                          i == m_trackSet ? Color{55, 95, 145, 255} : Color{35, 40, 38, 255}, true);
        renderer.drawText(std::to_string(i + 1), x + 6.0f, 232.0f, 1.0f, Color::white());
    }

    renderer.drawText("DENSITY", 250.0f, 246.0f, 1.0f, Color::cyan());
    const char* densities[] = {"L", "N", "H"};
    for (int i = 0; i < 3; ++i) {
        const float x = 302.0f + i * 15.0f;
        renderer.drawRect(x, 242.0f, 14.0f, 20.0f,
                          i == static_cast<int>(m_bloonDensity) ? Color{55, 95, 145, 255} : Color{35, 40, 38, 255}, true);
        renderer.drawText(densities[i], x + 4.0f, 248.0f, 1.0f, Color::white());
    }

    if (!m_status.empty()) {
        renderer.drawRect(8.0f, 252.0f, 238.0f, 14.0f, {0, 0, 0, 190}, true);
        renderer.drawText(m_status, 12.0f, 256.0f, 1.0f,
                          m_statusGood ? Color::green() : Color::red());
    }
}

} // namespace btd4
