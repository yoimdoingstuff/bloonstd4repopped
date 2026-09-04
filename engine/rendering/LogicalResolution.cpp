#include "LogicalResolution.hpp"
#include <algorithm>

namespace btd4 {

Viewport LogicalResolution::calculateViewport(int windowWidth, int windowHeight) {
    if (windowWidth <= 0 || windowHeight <= 0) {
        return {0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT};
    }

    float windowAspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    Viewport vp;

    if (windowAspect >= ASPECT_RATIO) {
        // Window is wider than logical aspect -> Pillarbox (bars on left/right)
        vp.height = windowHeight;
        vp.width = static_cast<int>(windowHeight * ASPECT_RATIO);
        vp.x = (windowWidth - vp.width) / 2;
        vp.y = 0;
    } else {
        // Window is taller than logical aspect -> Letterbox (bars on top/bottom)
        vp.width = windowWidth;
        vp.height = static_cast<int>(windowWidth / ASPECT_RATIO);
        vp.x = 0;
        vp.y = (windowHeight - vp.height) / 2;
    }

    return vp;
}

bool LogicalResolution::screenToLogical(int screenX, int screenY, const Viewport& viewport, float& outLogicalX, float& outLogicalY) {
    if (viewport.width <= 0 || viewport.height <= 0) {
        return false;
    }

    // Offset relative to viewport origin
    int relX = screenX - viewport.x;
    int relY = screenY - viewport.y;

    // Check bounds
    if (relX < 0 || relX > viewport.width || relY < 0 || relY > viewport.height) {
        outLogicalX = std::clamp(static_cast<float>(relX) / static_cast<float>(viewport.width) * LOGICAL_WIDTH, 0.0f, static_cast<float>(LOGICAL_WIDTH));
        outLogicalY = std::clamp(static_cast<float>(relY) / static_cast<float>(viewport.height) * LOGICAL_HEIGHT, 0.0f, static_cast<float>(LOGICAL_HEIGHT));
        return false;
    }

    outLogicalX = (static_cast<float>(relX) / static_cast<float>(viewport.width)) * LOGICAL_WIDTH;
    outLogicalY = (static_cast<float>(relY) / static_cast<float>(viewport.height)) * LOGICAL_HEIGHT;
    return true;
}

void LogicalResolution::logicalToScreen(float logicalX, float logicalY, const Viewport& viewport, int& outScreenX, int& outScreenY) {
    outScreenX = viewport.x + static_cast<int>((logicalX / static_cast<float>(LOGICAL_WIDTH)) * viewport.width);
    outScreenY = viewport.y + static_cast<int>((logicalY / static_cast<float>(LOGICAL_HEIGHT)) * viewport.height);
}

} // namespace btd4
