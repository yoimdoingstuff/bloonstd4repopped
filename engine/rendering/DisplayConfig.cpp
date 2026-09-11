#include "DisplayConfig.hpp"
#include <algorithm>
#include <cmath>

namespace btd4 {

Resolution DisplayConfig::getProfileResolution(DisplayProfile profile) {
    switch (profile) {
        case DisplayProfile::PSP_480x272:           return {480, 272};
        case DisplayProfile::Desktop_360p_16x9:      return {640, 360};
        case DisplayProfile::Desktop_540p_16x9:      return {960, 540};
        case DisplayProfile::Desktop_720p_16x9:      return {1280, 720};
        case DisplayProfile::Desktop_1080p_16x9:     return {1920, 1080};
        case DisplayProfile::Desktop_480p_4x3:       return {640, 480};
        case DisplayProfile::Desktop_600p_4x3:       return {800, 600};
        case DisplayProfile::Desktop_768p_4x3:       return {1024, 768};
        case DisplayProfile::iPad_HD_1024x768:       return {1024, 768};
        case DisplayProfile::iPad_Retina_2048x1536:  return {2048, 1536};
        case DisplayProfile::Custom:                 return {480, 272};
    }
    return {480, 272};
}

const char* DisplayConfig::getProfileName(DisplayProfile profile) {
    switch (profile) {
        case DisplayProfile::PSP_480x272:           return "PSP (480x272)";
        case DisplayProfile::Desktop_360p_16x9:      return "Desktop 360p 16:9 (640x360)";
        case DisplayProfile::Desktop_540p_16x9:      return "Desktop 540p 16:9 (960x540)";
        case DisplayProfile::Desktop_720p_16x9:      return "Desktop 720p 16:9 (1280x720)";
        case DisplayProfile::Desktop_1080p_16x9:     return "Desktop 1080p 16:9 (1920x1080)";
        case DisplayProfile::Desktop_480p_4x3:       return "Desktop 480p 4:3 (640x480)";
        case DisplayProfile::Desktop_600p_4x3:       return "Desktop 600p 4:3 (800x600)";
        case DisplayProfile::Desktop_768p_4x3:       return "Desktop 768p 4:3 (1024x768)";
        case DisplayProfile::iPad_HD_1024x768:       return "iPad HD (1024x768)";
        case DisplayProfile::iPad_Retina_2048x1536:  return "iPad Retina (2048x1536)";
        case DisplayProfile::Custom:                 return "Custom Resolution";
    }
    return "Unknown";
}

Viewport DisplayConfig::calculateViewport(int windowWidth, int windowHeight,
                                          ScalingMode scalingMode,
                                          Resolution logicalRes) {
    if (windowWidth <= 0 || windowHeight <= 0 || logicalRes.width <= 0 || logicalRes.height <= 0) {
        return {0, 0, 0, 0};
    }

    if (scalingMode == ScalingMode::Stretch) {
        return {0, 0, windowWidth, windowHeight};
    }

    if (scalingMode == ScalingMode::IntegerOnly) {
        int scaleX = windowWidth / logicalRes.width;
        int scaleY = windowHeight / logicalRes.height;
        int scale = std::max(1, std::min(scaleX, scaleY));

        int vpW = logicalRes.width * scale;
        int vpH = logicalRes.height * scale;
        int vpX = (windowWidth - vpW) / 2;
        int vpY = (windowHeight - vpH) / 2;
        return {vpX, vpY, vpW, vpH};
    }

    // Default: Letterbox / Pillarbox preserving aspect ratio
    float logicalAspect = static_cast<float>(logicalRes.width) / static_cast<float>(logicalRes.height);
    float windowAspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

    Viewport vp;
    if (windowAspect > logicalAspect) {
        // Pillarbox
        vp.height = windowHeight;
        vp.width = static_cast<int>(std::round(static_cast<float>(windowHeight) * logicalAspect));
        vp.x = (windowWidth - vp.width) / 2;
        vp.y = 0;
    } else {
        // Letterbox
        vp.width = windowWidth;
        vp.height = static_cast<int>(std::round(static_cast<float>(windowWidth) / logicalAspect));
        vp.x = 0;
        vp.y = (windowHeight - vp.height) / 2;
    }
    return vp;
}

bool DisplayConfig::screenToLogical(int screenX, int screenY,
                                    const Viewport& viewport,
                                    Resolution logicalResolution,
                                    float& outX, float& outY) {
    if (screenX < viewport.x || screenX >= viewport.x + viewport.width ||
        screenY < viewport.y || screenY >= viewport.y + viewport.height) {
        outX = -1.0f;
        outY = -1.0f;
        return false;
    }

    float normX = static_cast<float>(screenX - viewport.x) / static_cast<float>(viewport.width);
    float normY = static_cast<float>(screenY - viewport.y) / static_cast<float>(viewport.height);

    outX = normX * static_cast<float>(logicalResolution.width);
    outY = normY * static_cast<float>(logicalResolution.height);
    return true;
}

void DisplayConfig::logicalToScreen(float logicalX, float logicalY,
                                    const Viewport& viewport,
                                    Resolution logicalResolution,
                                    int& outScreenX, int& outScreenY) {
    float normX = logicalX / static_cast<float>(logicalResolution.width);
    float normY = logicalY / static_cast<float>(logicalResolution.height);

    outScreenX = viewport.x + static_cast<int>(std::round(normX * static_cast<float>(viewport.width)));
    outScreenY = viewport.y + static_cast<int>(std::round(normY * static_cast<float>(viewport.height)));
}

} // namespace btd4
