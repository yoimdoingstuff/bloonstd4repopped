#include "AssetManager.hpp"
#include "../rendering/DebugRenderer.hpp"
#include <filesystem>
#include <cmath>

namespace btd4 {

namespace fs = std::filesystem;

AssetManager& AssetManager::instance() {
    static AssetManager s_instance;
    return s_instance;
}

AssetManager::AssetManager() {
    setupFallbackColors();
}

void AssetManager::setupFallbackColors() {
    // 1:1 Color fallbacks for Bloons
    m_fallbackColors["bloon_red"]     = {255, 0, 0};
    m_fallbackColors["bloon_blue"]    = {30, 144, 255};
    m_fallbackColors["bloon_green"]   = {50, 205, 50};
    m_fallbackColors["bloon_yellow"]  = {255, 215, 0};
    m_fallbackColors["bloon_pink"]    = {255, 105, 180};
    m_fallbackColors["bloon_black"]   = {30, 30, 30};
    m_fallbackColors["bloon_white"]   = {240, 248, 255};
    m_fallbackColors["bloon_lead"]    = {169, 169, 169};
    m_fallbackColors["bloon_rainbow"] = {238, 130, 238};
    m_fallbackColors["bloon_ceramic"] = {160, 82, 45};
    m_fallbackColors["bloon_moab"]    = {0, 0, 139};

    // 1:1 Color fallbacks for Towers
    m_fallbackColors["tower_dart_monkey"] = {139, 69, 19};
    m_fallbackColors["tower_tack_shooter"] = {255, 69, 0};
    m_fallbackColors["tower_sniper_monkey"] = {85, 107, 47};
    m_fallbackColors["tower_boomerang"]   = {210, 105, 30};
    m_fallbackColors["tower_bomb_tower"]  = {70, 70, 70};
    m_fallbackColors["tower_super_monkey"] = {255, 215, 0};
}

bool AssetManager::initialize(const IFileSystem& fsBridge, const std::string& dataDirectory) {
    m_dataDir = dataDirectory;
    std::string manifestFile = m_dataDir + "/manifest.json";

    std::string err;
    if (m_manifest.loadFromFile(fsBridge, manifestFile, err)) {
        m_hasManifest = true;
        return true;
    }

    // Try fallback placeholder manifest
    std::string placeholderManifest = "assets/placeholder/manifest.json";
    if (m_manifest.loadFromFile(fsBridge, placeholderManifest, err)) {
        m_dataDir = "assets/placeholder";
        m_hasManifest = true;
        return true;
    }

    // Fallback mode enabled: assets will use built-in procedural visuals
    m_hasManifest = false;
    return true;
}

std::string AssetManager::resolveTexturePath(const std::string& assetId) const {
    if (m_hasManifest) {
        auto it = m_manifest.textures.find(assetId);
        if (it != m_manifest.textures.end()) {
            return m_dataDir + "/" + it->second;
        }
    }
    return ""; // Fallback will be used
}

std::string AssetManager::resolveAudioPath(const std::string& assetId) const {
    if (m_hasManifest) {
        auto it = m_manifest.audio.find(assetId);
        if (it != m_manifest.audio.end()) {
            return m_dataDir + "/" + it->second;
        }
    }
    return "";
}

bool AssetManager::isUsingFallback(const std::string& assetId) const {
    return resolveTexturePath(assetId).empty();
}

PlaceholderColor AssetManager::getPlaceholderColor(const std::string& assetId) const {
    auto it = m_fallbackColors.find(assetId);
    if (it != m_fallbackColors.end()) {
        return it->second;
    }
    return {200, 200, 200}; // Default neutral gray
}

size_t AssetManager::preloadTextures(IRenderer& renderer) {
    if (!m_hasManifest) {
        return 0;
    }
    size_t loaded = 0;
    for (const auto& pair : m_manifest.textures) {
        std::string fullPath = m_dataDir + "/" + pair.second;
        if (renderer.loadTexture(pair.first, fullPath)) {
            loaded++;
        }
    }
    return loaded;
}

std::string AssetManager::getBloonAssetId(BloonType type) {
    switch (type) {
        case BloonType::Red:     return "bloon_red";
        case BloonType::Blue:    return "bloon_blue";
        case BloonType::Green:   return "bloon_green";
        case BloonType::Yellow:  return "bloon_yellow";
        case BloonType::Pink:    return "bloon_pink";
        case BloonType::Black:   return "bloon_black";
        case BloonType::White:   return "bloon_white";
        case BloonType::Lead:    return "bloon_lead";
        case BloonType::Rainbow: return "bloon_rainbow";
        case BloonType::Ceramic: return "bloon_ceramic";
        case BloonType::MOAB:    return "bloon_moab";
        default:                 return "";
    }
}

std::string AssetManager::getTowerAssetId(TowerType type) {
    switch (type) {
        case TowerType::DartMonkey:       return "tower_dart_monkey";
        case TowerType::TackShooter:      return "tower_tack_shooter";
        case TowerType::SniperMonkey:     return "tower_sniper_monkey";
        case TowerType::BoomerangThrower: return "tower_boomerang";
        case TowerType::BombTower:        return "tower_bomb_tower";
        case TowerType::SuperMonkey:      return "tower_super_monkey";
        default:                          return "";
    }
}

std::string AssetManager::getProjectileAssetId(ProjectileType type) {
    switch (type) {
        case ProjectileType::Dart:       return "projectile_dart";
        case ProjectileType::Tack:       return "projectile_tack";
        case ProjectileType::Bomb:       return "projectile_bomb";
        case ProjectileType::Boomerang:  return "projectile_boomerang";
        case ProjectileType::SniperShot: return "projectile_bullet";
        case ProjectileType::Laser:      return "projectile_laser";
        case ProjectileType::Plasma:     return "projectile_plasma";
        default:                         return "";
    }
}

void AssetManager::drawBloon(IRenderer& renderer, const Bloon& bloon) const {
    if (!bloon.active) return;
    std::string assetId = getBloonAssetId(bloon.type);
    if (renderer.hasTexture(assetId)) {
        renderer.drawSprite(assetId, bloon.x - bloon.radius, bloon.y - bloon.radius, bloon.radius * 2.0f, bloon.radius * 2.0f);
    } else {
        PlaceholderColor pc = getPlaceholderColor(assetId);
        Color c{pc.r, pc.g, pc.b, 255};
        renderer.drawCircle(bloon.x, bloon.y - 1.0f, bloon.radius, c, true);
        renderer.drawCircle(bloon.x, bloon.y - 1.0f, bloon.radius, Color::black(), false);
        renderer.drawCircle(bloon.x - bloon.radius * 0.35f, bloon.y - bloon.radius * 0.45f, 1.5f, Color::white(), true);
        renderer.drawRect(bloon.x - 1.0f, bloon.y + bloon.radius - 1.0f, 2.0f, 2.0f, c, true);
    }
}

void AssetManager::drawTower(IRenderer& renderer, const Tower& tower, bool isSelected) const {
    if (isSelected) {
        DebugRenderer::drawTowerRange(renderer, tower.x(), tower.y(), tower.range());
    }
    std::string assetId = getTowerAssetId(tower.type());
    if (renderer.hasTexture(assetId)) {
        renderer.drawSprite(assetId, tower.x() - 16.0f, tower.y() - 16.0f, 32.0f, 32.0f);
    } else {
        PlaceholderColor pc = getPlaceholderColor(assetId);
        Color bodyColor{pc.r, pc.g, pc.b, 255};
        renderer.drawCircle(tower.x(), tower.y(), 12.0f, bodyColor, true);
        renderer.drawCircle(tower.x(), tower.y(), 12.0f, Color::black(), false);
        renderer.drawCircle(tower.x(), tower.y(), 4.0f, Color::white(), true);
        renderer.drawCircle(tower.x(), tower.y(), 2.0f, Color::black(), true);
    }
}

void AssetManager::drawProjectile(IRenderer& renderer, const Projectile& proj) const {
    if (!proj.active) return;
    std::string assetId = getProjectileAssetId(proj.type);
    if (renderer.hasTexture(assetId)) {
        renderer.drawSprite(assetId, proj.x - 4.0f, proj.y - 4.0f, 8.0f, 8.0f);
    } else {
        if (proj.type == ProjectileType::Bomb) {
            renderer.drawCircle(proj.x, proj.y, 4.0f, Color::black(), true);
            renderer.drawCircle(proj.x, proj.y, 4.0f, Color::red(), false);
        } else if (proj.type == ProjectileType::Plasma) {
            renderer.drawCircle(proj.x, proj.y, 5.0f, Color::cyan(), true);
        } else {
            float len = 6.0f;
            float speed = std::sqrt(proj.vx * proj.vx + proj.vy * proj.vy);
            float dirX = (speed > 0.001f) ? (proj.vx / speed) : 1.0f;
            float dirY = (speed > 0.001f) ? (proj.vy / speed) : 0.0f;
            renderer.drawLine(proj.x, proj.y, proj.x - dirX * len, proj.y - dirY * len, Color::yellow());
        }
    }
}

void AssetManager::drawMap(IRenderer& renderer, const Map& map) const {
    // Background grass
    renderer.drawRect(0.0f, 0.0f, 480.0f, 272.0f, {34, 139, 34, 255}, true);

    // Blocked regions / obstacles
    for (const auto& br : map.blockedRegions()) {
        renderer.drawRect(br.x, br.y, br.w, br.h, {46, 117, 46, 255}, true);
        renderer.drawRect(br.x, br.y, br.w, br.h, {25, 80, 25, 255}, false);
    }

    // Dirt road
    for (const auto& path : map.paths()) {
        const auto& waypoints = path.waypoints();
        for (size_t i = 0; i + 1 < waypoints.size(); ++i) {
            for (float offset = -8.0f; offset <= 8.0f; offset += 2.0f) {
                renderer.drawLine(waypoints[i].x + offset, waypoints[i].y,
                                  waypoints[i + 1].x + offset, waypoints[i + 1].y,
                                  {210, 180, 140, 255});
            }
            // Road border lines
            renderer.drawLine(waypoints[i].x - 9.0f, waypoints[i].y,
                              waypoints[i + 1].x - 9.0f, waypoints[i + 1].y,
                              {160, 130, 95, 255});
            renderer.drawLine(waypoints[i].x + 9.0f, waypoints[i].y,
                              waypoints[i + 1].x + 9.0f, waypoints[i + 1].y,
                              {160, 130, 95, 255});
        }
    }
}

void AssetManager::drawHUD(IRenderer& renderer, const Economy& economy, int currentRound,
                           size_t totalRounds, double fps, TowerType selectedPlacementType,
                           bool hasPlacement) const {
    // Top HUD bar
    renderer.drawRect(0.0f, 0.0f, 400.0f, 22.0f, {0, 0, 0, 180}, true);

    std::string livesText = "LIVES: " + std::to_string(economy.lives());
    renderer.drawText(livesText, 8.0f, 6.0f, 1.0f, Color::red());

    std::string cashText = "CASH: $" + std::to_string(economy.cash());
    renderer.drawText(cashText, 100.0f, 6.0f, 1.0f, Color::yellow());

    std::string roundText = "ROUND: " + std::to_string(currentRound) + "/" + std::to_string(totalRounds);
    renderer.drawText(roundText, 210.0f, 6.0f, 1.0f, Color::white());

    std::string fpsText = "FPS: " + std::to_string(static_cast<int>(std::round(fps)));
    renderer.drawText(fpsText, 340.0f, 6.0f, 1.0f, Color::green());

    // Right sidebar for towers (mobile-port style evolution)
    renderer.drawRect(400.0f, 0.0f, 80.0f, 272.0f, {20, 20, 20, 230}, true);
    renderer.drawLine(400.0f, 0.0f, 400.0f, 272.0f, {60, 60, 60, 255});
    renderer.drawText("TOWERS", 412.0f, 6.0f, 1.0f, Color::cyan());

    static const TowerType availableTowers[] = {
        TowerType::DartMonkey,
        TowerType::TackShooter,
        TowerType::BombTower,
        TowerType::BoomerangThrower,
        TowerType::SniperMonkey,
        TowerType::SuperMonkey
    };

    float btnY = 22.0f;
    for (TowerType tt : availableTowers) {
        auto stats = getTowerBaseStats(tt);
        bool isSel = (hasPlacement && selectedPlacementType == tt);
        bool canAfford = economy.canAfford(stats.cost);

        Color btnBg = isSel ? Color{60, 100, 160, 255} : (canAfford ? Color{45, 45, 45, 255} : Color{30, 30, 30, 255});
        renderer.drawRect(404.0f, btnY, 72.0f, 36.0f, btnBg, true);
        renderer.drawRect(404.0f, btnY, 72.0f, 36.0f, isSel ? Color::cyan() : Color{80, 80, 80, 255}, false);

        std::string shortName;
        switch (tt) {
            case TowerType::DartMonkey: shortName = "DART"; break;
            case TowerType::TackShooter: shortName = "TACK"; break;
            case TowerType::BombTower: shortName = "BOMB"; break;
            case TowerType::BoomerangThrower: shortName = "RANG"; break;
            case TowerType::SniperMonkey: shortName = "SNIP"; break;
            case TowerType::SuperMonkey: shortName = "SPER"; break;
        }

        renderer.drawText(shortName, 408.0f, btnY + 4.0f, 1.0f, isSel ? Color::white() : (canAfford ? Color::white() : Color{120, 120, 120, 255}));
        renderer.drawText("$" + std::to_string(stats.cost), 408.0f, btnY + 18.0f, 1.0f, canAfford ? Color::yellow() : Color::red());

        btnY += 40.0f;
    }
}

} // namespace btd4
