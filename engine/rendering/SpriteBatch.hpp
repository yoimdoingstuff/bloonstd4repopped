#pragma once

#include "IRenderer.hpp"
#include "TextureAtlas.hpp"
#include <vector>
#include <string>

namespace btd4 {

struct SpriteCommand {
    std::string textureKey;
    Rect srcRect{0.0f, 0.0f, 0.0f, 0.0f};
    Rect dstRect{0.0f, 0.0f, 0.0f, 0.0f};
    float rotationDegrees{0.0f};
    Color tint{Color::white()};
    int zOrder{0};
};

class SpriteBatch {
public:
    explicit SpriteBatch(size_t initialCapacity = 512);

    void begin();

    void draw(const std::string& textureKey, float x, float y, float w, float h,
              float rotationDegrees = 0.0f, const Color& tint = Color::white(), int zOrder = 0);

    void drawRegion(const std::string& textureKey, const Rect& srcRect, float x, float y, float w, float h,
                    float rotationDegrees = 0.0f, const Color& tint = Color::white(), int zOrder = 0);

    void drawAtlas(const TextureAtlas& atlas, const std::string& regionName, float x, float y, float w, float h,
                   float rotationDegrees = 0.0f, const Color& tint = Color::white(), int zOrder = 0);

    void end(IRenderer& renderer);

    size_t getCommandCount() const { return m_commands.size(); }

private:
    std::vector<SpriteCommand> m_commands;
};

} // namespace btd4
