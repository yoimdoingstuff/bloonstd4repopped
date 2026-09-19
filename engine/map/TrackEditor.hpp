#pragma once

#include "Map.hpp"
#include "../input/IInput.hpp"
#include "../rendering/IRenderer.hpp"
#include <array>
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

    static constexpr int MaxPaths = 4;

    void open(const Map& sourceMap);
    void close();

    bool isOpen() const { return m_open; }
    const Map& map() const { return m_map; }

    bool consumeApplyRequest(Map& outputMap);

    void update(const IInput& input, const PointerState& pointer);
    void render(IRenderer& renderer) const;

private:
    static constexpr int kColumns = 21;
    static constexpr int kRows = 14;
    static constexpr float kCellSize = 16.0f;
    static constexpr float kGridOriginX = 8.0f;
    static constexpr float kGridOriginY = 24.0f;

    bool m_open{false};
    bool m_applyRequested{false};
    Map m_map{"Track Editor"};
    Tool m_tool{Tool::Road};
    int m_activePath{0};
    uint8_t m_trackSet{0};
    BloonDensity m_bloonDensity{BloonDensity::Normal};
    std::string m_status{"Select a path and connect its edge entrance to an edge exit."};
    bool m_statusGood{true};

    std::array<bool, MaxPaths> m_hasStart{};
    std::array<bool, MaxPaths> m_hasFinish{};
    std::array<std::vector<int>, MaxPaths> m_pathCells;

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
    bool validateCurrent() const;
    void setStatus(const std::string& text, bool good);
};

} // namespace btd4
