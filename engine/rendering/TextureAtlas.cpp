#include "TextureAtlas.hpp"

namespace btd4 {

void TextureAtlas::addRegion(const std::string& name, const std::string& textureKey, const Rect& subRect, int origW, int origH) {
    AtlasRegion region;
    region.name = name;
    region.textureKey = textureKey;
    region.subRect = subRect;
    region.originalWidth = (origW > 0) ? origW : static_cast<int>(subRect.w);
    region.originalHeight = (origH > 0) ? origH : static_cast<int>(subRect.h);
    m_regions[name] = std::move(region);
}

bool TextureAtlas::hasRegion(const std::string& name) const {
    return m_regions.find(name) != m_regions.end();
}

const AtlasRegion* TextureAtlas::getRegion(const std::string& name) const {
    auto it = m_regions.find(name);
    if (it != m_regions.end()) {
        return &it->second;
    }
    return nullptr;
}

void TextureAtlas::clear() {
    m_regions.clear();
}

} // namespace btd4
