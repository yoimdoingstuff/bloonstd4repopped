#pragma once

#include "Map.hpp"
#include "../input/IInput.hpp"
#include "../rendering/IRenderer.hpp"
#include <string>
#include <vector>

namespace btd4 {

class TrackEditor {
public:
    enum class Tool {
        Road,
        Start,
        Finish,
        Erase
    };

    void open(const Map& sourceMap);
    void close();

    bool isOpen() const { return m_open; }
    const Map& map() const { return m_map; }

    // Returns true when the editor requested a playable handoff. The caller
    // owns applying the returned map to the active simulation.
    bool consumeApplyRequest(Map& outputMap);

    void update(const IInput& input, const PointerState& pointer);
    void render(IRenderer& renderer) const;

private:
    static constexpr int kColumns = 24;
    static constexpr int kRows = 14;
    static constexpr float kCellSize = 16.0f;
    static constexpr float kGridOriginX = 8.0f;
    static constexpr float kGridOriginY = 24.0f;

    bool m_open{false};
    bool m_applyRequested{false};
    Map m_map{"Track Editor"};
    Tool m_tool{Tool::Road};
    int m_trackStyle{0};
    std::string m_status{"Click a grid cell to start a track."};
    bool m_statusGood{true};

    bool m_hasStart{false};
    bool m_hasFinish{false};
    std::vector<int> m_pathCells;

    bool isInsideGrid(float x, float y) const;
    int cellFromPointer(float x, float y) const;
    int cellX(int cell) const { return cell % kColumns; }
    int cellY(int cell) const { return cell / kColumns; }
    float cellCenterX(int cell) const;
    float cellCenterY(int cell) const;
    bool isBoundaryCell(int cell) const;
    bool isAdjacent(int a, int b) const;
    int findCell(int x, int y) const;
    void rebuildMapFromCells();
    void rebuildCellsFromMap();
    bool setStartCell(int cell);
    bool setFinishCell(int cell);
    bool appendRoadCell(int cell);
    bool eraseCell(int cell);
    bool save();
    bool load();
    void setStatus(const std::string& text, bool good);
};

} // namespace btd4
