#include "DebugRenderer.hpp"
#include <cstdio>
#include <string>

namespace btd4 {

void DebugRenderer::drawTowerRange(IRenderer& renderer, float cx, float cy, float range,
                                   const Color& outlineColor,
                                   const Color& fillColor) {
    if (fillColor.a > 0) {
        renderer.drawCircle(cx, cy, range, fillColor, true);
    }
    renderer.drawCircle(cx, cy, range, outlineColor, false);
}

void DebugRenderer::drawBoundingBox(IRenderer& renderer, float x, float y, float w, float h,
                                    const Color& color, bool filled) {
    renderer.drawRect(x, y, w, h, color, filled);
}

void DebugRenderer::drawPath(IRenderer& renderer, const Path& path,
                             const Color& lineColor,
                             const Color& waypointColor) {
    const auto& waypoints = path.waypoints();
    if (waypoints.empty()) {
        return;
    }

    for (size_t i = 0; i + 1 < waypoints.size(); ++i) {
        renderer.drawLine(waypoints[i].x, waypoints[i].y,
                          waypoints[i + 1].x, waypoints[i + 1].y,
                          lineColor);
    }

    for (const auto& pt : waypoints) {
        renderer.drawCircle(pt.x, pt.y, 3.0f, waypointColor, true);
    }
}

void DebugRenderer::drawGrid(IRenderer& renderer, float cellSize, float mapWidth, float mapHeight,
                             const Color& color) {
    if (cellSize <= 0.0f) {
        return;
    }
    for (float x = 0.0f; x <= mapWidth; x += cellSize) {
        renderer.drawLine(x, 0.0f, x, mapHeight, color);
    }
    for (float y = 0.0f; y <= mapHeight; y += cellSize) {
        renderer.drawLine(0.0f, y, mapWidth, y, color);
    }
}

void DebugRenderer::drawFps(IRenderer& renderer, double fps, float x, float y) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "FPS: %.1f", fps);
    renderer.drawText(buf, x, y, 1.0f, Color::green());
}

} // namespace btd4
