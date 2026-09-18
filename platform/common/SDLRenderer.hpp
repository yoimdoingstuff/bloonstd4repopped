#pragma once

#include "../../engine/rendering/IRenderer.hpp"
#include <SDL.h>
#include <unordered_map>

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
    void drawCircle(float cx, float cy, float radius, const Color& color, bool filled = false) override;
    void drawText(const std::string& text, float x, float y, float scale, const Color& color) override;

    bool loadTexture(const std::string& key, const std::string& filePath) override;
    bool hasTexture(const std::string& key) const override;
    void drawSprite(const std::string& textureKey, float x, float y, float w, float h, float angleDegrees = 0.0f, const Color& tint = Color::white()) override;
    void drawSpriteRegion(const std::string& textureKey, const Rect& srcRect, float x, float y, float w, float h, float angleDegrees = 0.0f, const Color& tint = Color::white()) override;

    void onResize(int newWidth, int newHeight) override;

    SDL_Renderer* rawRenderer() const { return m_renderer; }

private:
    SDL_Window* m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};
    bool m_ownsWindow{false};
    bool m_ownsVideoSubsystem{false};
    bool m_imageSubsystemInitialized{false};
    Viewport m_currentViewport{0, 0, 480, 272};
    std::unordered_map<std::string, SDL_Texture*> m_textures;
};

} // namespace btd4
