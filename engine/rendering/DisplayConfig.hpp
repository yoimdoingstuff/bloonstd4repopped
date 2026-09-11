#pragma once

#include "IRenderer.hpp"
#include <string>

namespace btd4 {

enum class DisplayProfile {
    PSP_480x272,
    Desktop_360p_16x9,
    Desktop_540p_16x9,
    Desktop_720p_16x9,
    Desktop_1080p_16x9,
    Desktop_480p_4x3,
    Desktop_600p_4x3,
    Desktop_768p_4x3,
    iPad_HD_1024x768,
    iPad_Retina_2048x1536,
    Custom
};

enum class ScalingMode {
    LetterboxPillarbox, // Preserves aspect ratio with letterbox/pillarbox
    IntegerOnly,        // Crisp integer multiples (1x, 2x, 3x, etc.) centered
    Stretch             // Fills entire window without bars
};

struct Resolution {
    int width{480};
    int height{272};
};

class DisplayConfig {
public:
    static Resolution getProfileResolution(DisplayProfile profile);
    static const char* getProfileName(DisplayProfile profile);

    static Viewport calculateViewport(int windowWidth, int windowHeight,
                                      ScalingMode scalingMode,
                                      Resolution logicalResolution = {480, 272});

    static bool screenToLogical(int screenX, int screenY,
                                const Viewport& viewport,
                                Resolution logicalResolution,
                                float& outX, float& outY);

    static void logicalToScreen(float logicalX, float logicalY,
                                const Viewport& viewport,
                                Resolution logicalResolution,
                                int& outX, int& outY);
};

} // namespace btd4
