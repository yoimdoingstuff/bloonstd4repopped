#include "SpriteBatch.hpp"
#include <algorithm>

namespace btd4 {

SpriteBatch::SpriteBatch(size_t initialCapacity) {
    m_commands.reserve(initialCapacity);
}

void SpriteBatch::begin() {
    m_commands.clear();
}

void SpriteBatch::draw(const std::string& textureKey, float x, float y, float w, float h,
                       float rotationDegrees, const Color& tint, int zOrder) {
    drawRegion(textureKey, {0.0f, 0.0f, 0.0f, 0.0f}, x, y, w, h, rotationDegrees, tint, zOrder);
}

void SpriteBatch::drawRegion(const std::string& textureKey, const Rect& srcRect, float x, float y, float w, float h,
                             float rotationDegrees, const Color& tint, int zOrder) {
    SpriteCommand cmd;
    cmd.textureKey = textureKey;
    cmd.srcRect = srcRect;
    cmd.dstRect = {x, y, w, h};
    cmd.rotationDegrees = rotationDegrees;
    cmd.tint = tint;
    cmd.zOrder = zOrder;
    m_commands.push_back(std::move(cmd));
}

void SpriteBatch::drawAtlas(const TextureAtlas& atlas, const std::string& regionName, float x, float y, float w, float h,
                            float rotationDegrees, const Color& tint, int zOrder) {
    const AtlasRegion* reg = atlas.getRegion(regionName);
    if (!reg) {
        return;
    }
    drawRegion(reg->textureKey, reg->subRect, x, y, w, h, rotationDegrees, tint, zOrder);
}

void SpriteBatch::end(IRenderer& renderer) {
    if (m_commands.empty()) {
        return;
    }

    // Stable sort to respect zOrder, and group identical textures together to avoid texture switching
    std::stable_sort(m_commands.begin(), m_commands.end(), [](const SpriteCommand& a, const SpriteCommand& b) {
        if (a.zOrder != b.zOrder) {
            return a.zOrder < b.zOrder;
        }
        return a.textureKey < b.textureKey;
    });

    for (const auto& cmd : m_commands) {
        if (cmd.srcRect.w > 0.0f && cmd.srcRect.h > 0.0f) {
            renderer.drawSpriteRegion(cmd.textureKey, cmd.srcRect, cmd.dstRect.x, cmd.dstRect.y,
                                      cmd.dstRect.w, cmd.dstRect.h, cmd.rotationDegrees, cmd.tint);
        } else {
            renderer.drawSprite(cmd.textureKey, cmd.dstRect.x, cmd.dstRect.y,
                                cmd.dstRect.w, cmd.dstRect.h, cmd.rotationDegrees, cmd.tint);
        }
    }
}

} // namespace btd4
