#pragma once

#include "AssetManifest.hpp"
#include "../core/IFileSystem.hpp"
#include "../rendering/IRenderer.hpp"
#include "../game/Bloon.hpp"
#include "../game/Tower.hpp"
#include "../game/Projectile.hpp"
#include "../game/Economy.hpp"
#include "../rendering/TextureAtlas.hpp"
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
    const std::string& dataDirectory() const { return m_dataDir; }

    std::string resolveTexturePath(const std::string& assetId) const;
    std::string resolveAudioPath(const std::string& assetId) const;

    bool isUsingFallback(const std::string& assetId) const;
    PlaceholderColor getPlaceholderColor(const std::string& assetId) const;

    size_t preloadTextures(IRenderer& renderer);

    static std::string getBloonAssetId(BloonType type);
    static std::string getTowerAssetId(TowerType type);
    static std::string getProjectileAssetId(ProjectileType type);

    void drawBloon(IRenderer& renderer, const Bloon& bloon) const;
    void drawTower(IRenderer& renderer, const Tower& tower, bool isSelected = false) const;
    void drawProjectile(IRenderer& renderer, const Projectile& proj) const;
    void drawMap(IRenderer& renderer, const Map& map) const;
    void drawMainMenu(IRenderer& renderer, float pointerX = -1.0f, float pointerY = -1.0f) const;
    void drawHUD(IRenderer& renderer, const Economy& economy, int currentRound, size_t totalRounds,
                 double fps, TowerType selectedPlacementType, bool hasPlacement) const;
    bool drawGameUiRegion(IRenderer& renderer, const std::string& region,
                          float x, float y, float w, float h) const;

private:
    AssetManager();
    ~AssetManager() = default;

    std::string m_dataDir{"game_data"};
    AssetManifest m_manifest;
    bool m_hasManifest{false};

    std::unordered_map<std::string, PlaceholderColor> m_fallbackColors;
    TextureAtlas m_towersAtlas;
    TextureAtlas m_gameUiAtlas;
    TextureAtlas m_mainMenuAtlas;
    std::string m_towerSheetTextureId;
    std::string m_gameUiTextureId;
    std::string m_mainMenuTextureId;
    void setupFallbackColors();
    void loadRuntimeAtlases(const IFileSystem& fs);
    bool drawAtlasRegion(IRenderer& renderer, const TextureAtlas& atlas, const std::string& region,
                         float x, float y, float w, float h) const;
    const TextureAtlas* towerAtlas() const { return m_towersAtlas.regionCount() ? &m_towersAtlas : nullptr; }
    const TextureAtlas* gameUiAtlas() const { return m_gameUiAtlas.regionCount() ? &m_gameUiAtlas : nullptr; }
    const TextureAtlas* mainMenuAtlas() const { return m_mainMenuAtlas.regionCount() ? &m_mainMenuAtlas : nullptr; }
};

} // namespace btd4
