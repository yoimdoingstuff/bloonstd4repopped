#pragma once

#include "../../engine/rendering/IRenderer.hpp"
#include <SDL.h>

namespace btd4 {

class SDLRenderer : public IRenderer {
public:
    SDLRenderer();
    ~SDLRenderer() override;

    bool initializeWithWindow(SDL_Window* window);
    bool initialize(int windowWidth, int windowHeight) override;
    void shutdown() override;

    void beginFrame() override;
    void endFrame() override;

    void setViewport(const Viewport& viewport) override;
    void clear(const Color& color) override;

    void drawRect(float x, float y, float w, float h, const Color& color, bool filled = true) override;
    void drawLine(float x1, float y1, float x2, float y2, const Color& color) override;
    void drawText(const std::string& text, float x, float y, float scale, const Color& color) override;

    void onResize(int newWidth, int newHeight) override;

    SDL_Renderer* rawRenderer() const { return m_renderer; }

private:
    SDL_Window* m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};
    bool m_ownsWindow{false};
    bool m_ownsVideoSubsystem{false};
    Viewport m_currentViewport{0, 0, 480, 272};
};

} // namespace btd4
