#pragma once

#include "IRenderer.hpp"
#include "../map/Map.hpp"

namespace btd4 {

class DebugRenderer {
public:
    static void drawTowerRange(IRenderer& renderer, float cx, float cy, float range,
                               const Color& outlineColor = Color::cyan(),
                               const Color& fillColor = {0, 255, 255, 40});

    static void drawBoundingBox(IRenderer& renderer, float x, float y, float w, float h,
                                const Color& color = Color::red(), bool filled = false);

    static void drawPath(IRenderer& renderer, const Path& path,
                         const Color& lineColor = Color::yellow(),
                         const Color& waypointColor = Color::red());

    static void drawGrid(IRenderer& renderer, float cellSize, float mapWidth, float mapHeight,
                         const Color& color = {60, 60, 60, 150});

    static void drawFps(IRenderer& renderer, double fps, float x = 10.0f, float y = 10.0f);
};

} // namespace btd4
