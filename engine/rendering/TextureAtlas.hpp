#pragma once

#include "IRenderer.hpp"
#include <string>
#include <unordered_map>

namespace btd4 {

struct AtlasRegion {
    std::string name;
    std::string textureKey;
    Rect subRect{0.0f, 0.0f, 0.0f, 0.0f};
    int originalWidth{0};
    int originalHeight{0};
};

class TextureAtlas {
public:
    TextureAtlas() = default;

    void addRegion(const std::string& name, const std::string& textureKey, const Rect& subRect, int origW = 0, int origH = 0);
    bool hasRegion(const std::string& name) const;
    const AtlasRegion* getRegion(const std::string& name) const;
    size_t regionCount() const { return m_regions.size(); }

    void clear();

private:
    std::unordered_map<std::string, AtlasRegion> m_regions;
};

} // namespace btd4
