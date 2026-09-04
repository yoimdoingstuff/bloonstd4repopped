#pragma once

#include "IRenderer.hpp"

namespace btd4 {

class LogicalResolution {
public:
    static constexpr int LOGICAL_WIDTH = 480;
    static constexpr int LOGICAL_HEIGHT = 272;
    static constexpr float ASPECT_RATIO = static_cast<float>(LOGICAL_WIDTH) / static_cast<float>(LOGICAL_HEIGHT);

    static Viewport calculateViewport(int windowWidth, int windowHeight);

    // Screen pixel (e.g. mouse cursor) to logical coordinate (0..480, 0..272)
    static bool screenToLogical(int screenX, int screenY, const Viewport& viewport, float& outLogicalX, float& outLogicalY);

    // Logical coordinate to screen pixel
    static void logicalToScreen(float logicalX, float logicalY, const Viewport& viewport, int& outScreenX, int& outScreenY);
};

} // namespace btd4
