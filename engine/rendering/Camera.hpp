#pragma once

#include "IRenderer.hpp"

namespace btd4 {

class Camera {
public:
    Camera(float viewportWidth = 480.0f, float viewportHeight = 272.0f);

    void setPosition(float x, float y);
    void move(float dx, float dy);
    float getX() const { return m_x; }
    float getY() const { return m_y; }

    void setZoom(float zoom);
    float getZoom() const { return m_zoom; }

    void setViewportSize(float width, float height);
    float getViewportWidth() const { return m_viewportWidth; }
    float getViewportHeight() const { return m_viewportHeight; }

    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void screenToWorld(float sx, float sy, float& wx, float& wy) const;

    Rect getVisibleBounds() const;
    bool isVisible(const Rect& worldRect) const;

    void clampToBounds(const Rect& worldBounds);

private:
    float m_x{240.0f};
    float m_y{136.0f};
    float m_zoom{1.0f};
    float m_viewportWidth{480.0f};
    float m_viewportHeight{272.0f};
};

} // namespace btd4
