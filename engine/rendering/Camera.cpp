#include "Camera.hpp"
#include <algorithm>

namespace btd4 {

Camera::Camera(float viewportWidth, float viewportHeight)
    : m_viewportWidth(viewportWidth), m_viewportHeight(viewportHeight) {
    m_x = viewportWidth * 0.5f;
    m_y = viewportHeight * 0.5f;
}

void Camera::setPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

void Camera::move(float dx, float dy) {
    m_x += dx;
    m_y += dy;
}

void Camera::setZoom(float zoom) {
    m_zoom = std::max(0.05f, zoom);
}

void Camera::setViewportSize(float width, float height) {
    m_viewportWidth = std::max(1.0f, width);
    m_viewportHeight = std::max(1.0f, height);
}

void Camera::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = (wx - m_x) * m_zoom + (m_viewportWidth * 0.5f);
    sy = (wy - m_y) * m_zoom + (m_viewportHeight * 0.5f);
}

void Camera::screenToWorld(float sx, float sy, float& wx, float& wy) const {
    wx = (sx - (m_viewportWidth * 0.5f)) / m_zoom + m_x;
    wy = (sy - (m_viewportHeight * 0.5f)) / m_zoom + m_y;
}

Rect Camera::getVisibleBounds() const {
    float halfW = (m_viewportWidth * 0.5f) / m_zoom;
    float halfH = (m_viewportHeight * 0.5f) / m_zoom;
    return Rect{m_x - halfW, m_y - halfH, halfW * 2.0f, halfH * 2.0f};
}

bool Camera::isVisible(const Rect& worldRect) const {
    Rect view = getVisibleBounds();
    return !(worldRect.x + worldRect.w < view.x ||
             worldRect.x > view.x + view.w ||
             worldRect.y + worldRect.h < view.y ||
             worldRect.y > view.y + view.h);
}

void Camera::clampToBounds(const Rect& worldBounds) {
    Rect view = getVisibleBounds();
    if (view.w >= worldBounds.w) {
        m_x = worldBounds.x + worldBounds.w * 0.5f;
    } else {
        float minX = worldBounds.x + view.w * 0.5f;
        float maxX = worldBounds.x + worldBounds.w - view.w * 0.5f;
        m_x = std::clamp(m_x, minX, maxX);
    }

    if (view.h >= worldBounds.h) {
        m_y = worldBounds.y + worldBounds.h * 0.5f;
    } else {
        float minY = worldBounds.y + view.h * 0.5f;
        float maxY = worldBounds.y + worldBounds.h - view.h * 0.5f;
        m_y = std::clamp(m_y, minY, maxY);
    }
}

} // namespace btd4
