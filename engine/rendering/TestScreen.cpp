#include "TestScreen.hpp"
#include <iomanip>
#include <sstream>

namespace btd4 {

TestScreen::TestScreen() {
}

void TestScreen::setCursorPosition(float logicalX, float logicalY) {
    m_cursorX = logicalX;
    m_cursorY = logicalY;
}

void TestScreen::update(double deltaTime) {
    // Update bouncing box
    m_boxX += static_cast<float>(m_boxVX * deltaTime);
    m_boxY += static_cast<float>(m_boxVY * deltaTime);

    if (m_boxX <= 0.0f) {
        m_boxX = 0.0f;
        m_boxVX = -m_boxVX;
    } else if (m_boxX + m_boxSize >= LogicalResolution::LOGICAL_WIDTH) {
        m_boxX = LogicalResolution::LOGICAL_WIDTH - m_boxSize;
        m_boxVX = -m_boxVX;
    }

    if (m_boxY <= 0.0f) {
        m_boxY = 0.0f;
        m_boxVY = -m_boxVY;
    } else if (m_boxY + m_boxSize >= LogicalResolution::LOGICAL_HEIGHT) {
        m_boxY = LogicalResolution::LOGICAL_HEIGHT - m_boxSize;
        m_boxVY = -m_boxVY;
    }
}

void TestScreen::render(IRenderer& renderer, double fps) {
    // Clear logical canvas area
    renderer.clear(Color{25, 30, 45, 255});

    // Draw background grid (32x32 cells)
    Color gridColor{40, 50, 70, 255};
    for (int x = 0; x <= LogicalResolution::LOGICAL_WIDTH; x += 32) {
        renderer.drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x), static_cast<float>(LogicalResolution::LOGICAL_HEIGHT), gridColor);
    }
    for (int y = 0; y <= LogicalResolution::LOGICAL_HEIGHT; y += 32) {
        renderer.drawLine(0.0f, static_cast<float>(y), static_cast<float>(LogicalResolution::LOGICAL_WIDTH), static_cast<float>(y), gridColor);
    }

    // Outer boundary (480x272 safe area border)
    renderer.drawRect(0.0f, 0.0f, static_cast<float>(LogicalResolution::LOGICAL_WIDTH), static_cast<float>(LogicalResolution::LOGICAL_HEIGHT), Color::yellow(), false);

    // Bouncing box
    renderer.drawRect(m_boxX, m_boxY, m_boxSize, m_boxSize, Color::cyan(), true);
    renderer.drawRect(m_boxX, m_boxY, m_boxSize, m_boxSize, Color::white(), false);

    // Cursor indicator
    if (m_cursorX >= 0.0f && m_cursorY >= 0.0f) {
        renderer.drawRect(m_cursorX - 4.0f, m_cursorY - 4.0f, 8.0f, 8.0f, Color::red(), true);
    }

    // Header and FPS text
    std::ostringstream ss;
    ss << "FPS: " << std::fixed << std::setprecision(1) << fps << " (480x272)";
    renderer.drawText("BTD4 REPOPPED - TEST SCREEN", 12.0f, 10.0f, 1.0f, Color::white());
    renderer.drawText(ss.str(), 12.0f, 26.0f, 1.0f, Color::green());
}

} // namespace btd4
