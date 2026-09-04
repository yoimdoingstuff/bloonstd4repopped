#pragma once

#include <cstdint>
#include <string>

namespace btd4 {

struct Color {
    uint8_t r{255};
    uint8_t g{255};
    uint8_t b{255};
    uint8_t a{255};

    static constexpr Color white()  { return {255, 255, 255, 255}; }
    static constexpr Color black()  { return {0, 0, 0, 255}; }
    static constexpr Color red()    { return {255, 0, 0, 255}; }
    static constexpr Color green()  { return {0, 255, 0, 255}; }
    static constexpr Color blue()   { return {0, 0, 255, 255}; }
    static constexpr Color yellow() { return {255, 255, 0, 255}; }
    static constexpr Color cyan()   { return {0, 255, 255, 255}; }
    static constexpr Color darkGray() { return {40, 40, 40, 255}; }
};

struct Rect {
    float x{0.0f};
    float y{0.0f};
    float w{0.0f};
    float h{0.0f};
};

struct Viewport {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual bool initialize(int windowWidth, int windowHeight) = 0;
    virtual void shutdown() = 0;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual void setViewport(const Viewport& viewport) = 0;
    virtual void clear(const Color& color) = 0;

    virtual void drawRect(float x, float y, float w, float h, const Color& color, bool filled = true) = 0;
    virtual void drawLine(float x1, float y1, float x2, float y2, const Color& color) = 0;
    virtual void drawText(const std::string& text, float x, float y, float scale, const Color& color) = 0;

    virtual void onResize(int newWidth, int newHeight) = 0;
};

} // namespace btd4
