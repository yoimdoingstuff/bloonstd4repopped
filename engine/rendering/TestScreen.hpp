#pragma once

#include "IRenderer.hpp"
#include "LogicalResolution.hpp"

namespace btd4 {

class TestScreen {
public:
    TestScreen();

    void update(double deltaTime);
    void render(IRenderer& renderer, double fps);

    // Mouse / Cursor position for visual feedback
    void setCursorPosition(float logicalX, float logicalY);

private:
    float m_boxX{100.0f};
    float m_boxY{100.0f};
    float m_boxVX{120.0f};
    float m_boxVY{90.0f};
    float m_boxSize{32.0f};

    float m_cursorX{-10.0f};
    float m_cursorY{-10.0f};
};

} // namespace btd4
