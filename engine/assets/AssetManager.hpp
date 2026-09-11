#pragma once

#include "AssetManifest.hpp"
#include "../core/IFileSystem.hpp"
#include "../rendering/IRenderer.hpp"
#include "../game/Bloon.hpp"
#include "../game/Tower.hpp"
#include "../game/Projectile.hpp"
#include "../game/Economy.hpp"
#include "../map/Map.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace btd4 {

struct PlaceholderColor {
    uint8_t r{255};
    uint8_t g{255};
    uint8_t b{255};
};

class AssetManager {
public:
    static AssetManager& instance();

    bool initialize(const IFileSystem& fs, const std::string& dataDirectory = "game_data");

    bool hasManifest() const { return m_hasManifest; }
    const AssetManifest& manifest() const { return m_manifest; }

    std::string resolveTexturePath(const std::string& assetId) const;
    std::string resolveAudioPath(const std::string& assetId) const;

    // Placeholder procedural fallbacks
    bool isUsingFallback(const std::string& assetId) const;
    PlaceholderColor getPlaceholderColor(const std::string& assetId) const;

    // Preloads all manifest textures into the renderer
    size_t preloadTextures(IRenderer& renderer);

    // Asset ID resolvers
    static std::string getBloonAssetId(BloonType type);
    static std::string getTowerAssetId(TowerType type);
    static std::string getProjectileAssetId(ProjectileType type);

    // Entity rendering with sprite or procedural fallback
    void drawBloon(IRenderer& renderer, const Bloon& bloon) const;
    void drawTower(IRenderer& renderer, const Tower& tower, bool isSelected = false) const;
    void drawProjectile(IRenderer& renderer, const Projectile& proj) const;
    void drawMap(IRenderer& renderer, const Map& map) const;
    void drawHUD(IRenderer& renderer, const Economy& economy, int currentRound, size_t totalRounds,
                 double fps, TowerType selectedPlacementType, bool hasPlacement) const;

private:
    AssetManager();
    ~AssetManager() = default;

    std::string m_dataDir{"game_data"};
    AssetManifest m_manifest;
    bool m_hasManifest{false};

    std::unordered_map<std::string, PlaceholderColor> m_fallbackColors;
    void setupFallbackColors();
};

} // namespace btd4
