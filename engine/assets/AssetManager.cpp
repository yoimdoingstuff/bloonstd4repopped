#include "AssetManager.hpp"
#include "../rendering/DebugRenderer.hpp"
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <cctype>

namespace btd4 {
namespace fs = std::filesystem;

namespace {
std::string lowerId(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool conflictsWithFamily(const std::string& candidate, const std::string& logicalId) {
    const std::string id = lowerId(candidate);
    if (logicalId.rfind("projectile_", 0) == 0) {
        return id.find("tower") != std::string::npos || id.find("monkey") != std::string::npos ||
               id.find("bloon") != std::string::npos || id.find("background") != std::string::npos ||
               id.find("map") != std::string::npos;
    }
    if (logicalId.rfind("bloon_", 0) == 0) {
        return id.find("tower") != std::string::npos || id.find("projectile") != std::string::npos ||
               id.find("monkey") != std::string::npos || id.find("background") != std::string::npos ||
               id.find("map") != std::string::npos;
    }
    if (logicalId.rfind("tower_", 0) == 0) {
        return id.find("projectile") != std::string::npos || id.find("bloon") != std::string::npos ||
               id.find("background") != std::string::npos || id.find("map") != std::string::npos;
    }
    return false;
}

std::string findImportedTextureId(const AssetManifest& manifest, const IRenderer& renderer, const std::string& logicalId) {
    const std::string wanted = lowerId(logicalId);
    std::string token = wanted;
    if (wanted.rfind("tower_", 0) == 0) token = wanted.substr(6);
    else if (wanted.rfind("bloon_", 0) == 0) token = wanted.substr(6);
    else if (wanted.rfind("projectile_", 0) == 0) token = wanted.substr(11);
    if (token.empty()) return {};

    std::string bestId;
    int bestScore = -1;
    if (renderer.hasTexture(logicalId)) {
        bestId = logicalId;
        bestScore = 50;
    }
    for (const auto& [id, path] : manifest.textures) {
        (void)path;
        if (!renderer.hasTexture(id) || conflictsWithFamily(id, logicalId)) continue;
        const std::string lowered = lowerId(id);
        const size_t tokenPos = lowered.find(token);
        if (tokenPos == std::string::npos) continue;
        int score = 10;
        if (lowered == token) score += 50;
        if (tokenPos == 0) score += 15;
        if (lowered.find(wanted) != std::string::npos) score += 30;
        if (lowered.find("@hd") != std::string::npos)
            score += (manifest.targetPlatform == "PSP" ? -40 : 35);
        if (lowered.find("@mobile") != std::string::npos)
            score += (manifest.targetPlatform == "PSP" ? 40 : 10);
        else if (lowered.find("@phone") != std::string::npos)
            score += (manifest.targetPlatform == "PSP" ? 35 : 10);
        if (lowered.find("@expansion") != std::string::npos)
            score += 5;
        if (logicalId.rfind("projectile_", 0) == 0 &&
            (lowered.find("projectile") != std::string::npos || lowered.find("shot") != std::string::npos || lowered.find("bullet") != std::string::npos)) score += 20;
        if (logicalId.rfind("bloon_", 0) == 0 && lowered.find("bloon") != std::string::npos) score += 20;
        if (logicalId.rfind("tower_", 0) == 0 &&
            (lowered.find("tower") != std::string::npos || lowered.find("monkey") != std::string::npos)) score += 20;
        if (score > bestScore) {
            bestScore = score;
            bestId = id;
        }
    }
    return bestId;
}
}

AssetManager& AssetManager::instance() { static AssetManager s_instance; return s_instance; }
AssetManager::AssetManager() { setupFallbackColors(); }

void AssetManager::setupFallbackColors() {
    m_fallbackColors["bloon_red"]={255,0,0}; m_fallbackColors["bloon_blue"]={30,144,255};
    m_fallbackColors["bloon_green"]={50,205,50}; m_fallbackColors["bloon_yellow"]={255,215,0};
    m_fallbackColors["bloon_pink"]={255,105,180}; m_fallbackColors["bloon_black"]={30,30,30};
    m_fallbackColors["bloon_white"]={240,248,255}; m_fallbackColors["bloon_lead"]={169,169,169};
    m_fallbackColors["bloon_rainbow"]={238,130,238}; m_fallbackColors["bloon_ceramic"]={160,82,45};
    m_fallbackColors["bloon_moab"]={0,0,139};
    m_fallbackColors["tower_dart_monkey"]={139,69,19}; m_fallbackColors["tower_tack_shooter"]={255,69,0};
    m_fallbackColors["tower_sniper_monkey"]={85,107,47}; m_fallbackColors["tower_boomerang"]={210,105,30};
    m_fallbackColors["tower_bomb_tower"]={70,70,70}; m_fallbackColors["tower_super_monkey"]={255,215,0};
}

bool AssetManager::initialize(const IFileSystem& fsBridge, const std::string& dataDirectory) {
    m_dataDir=dataDirectory; m_manifest=AssetManifest{}; m_hasManifest=false; std::string err;
    if(m_manifest.loadFromFile(fsBridge,m_dataDir+"/manifest.json",err)){m_hasManifest=true;return true;}
    std::error_code ec; fs::path root(m_dataDir);
    if(fs::is_directory(root,ec)){
        std::vector<fs::path> manifests;
        for(const auto& entry:fs::directory_iterator(root,fs::directory_options::skip_permission_denied,ec)){if(ec)break;if(!entry.is_directory(ec))continue;const fs::path manifest=entry.path()/"manifest.json";if(fs::is_regular_file(manifest,ec))manifests.push_back(manifest);}
        std::sort(manifests.begin(),manifests.end());
        if(manifests.size()==1){const fs::path manifest=manifests.front();if(m_manifest.loadFromFile(fsBridge,manifest.generic_string(),err)){m_dataDir=manifest.parent_path().generic_string();m_hasManifest=true;return true;}}
    }
    if(m_manifest.loadFromFile(fsBridge,"assets/placeholder/manifest.json",err)){m_dataDir="assets/placeholder";m_hasManifest=true;return true;}
    return true;
}
std::string AssetManager::resolveTexturePath(const std::string& assetId) const {if(m_hasManifest){auto it=m_manifest.textures.find(assetId);if(it!=m_manifest.textures.end())return m_dataDir+"/"+it->second;}return "";}
std::string AssetManager::resolveAudioPath(const std::string& assetId) const {if(m_hasManifest){auto it=m_manifest.audio.find(assetId);if(it!=m_manifest.audio.end())return m_dataDir+"/"+it->second;}return "";}
bool AssetManager::isUsingFallback(const std::string& assetId) const{return resolveTexturePath(assetId).empty();}
PlaceholderColor AssetManager::getPlaceholderColor(const std::string& assetId) const{auto it=m_fallbackColors.find(assetId);return it!=m_fallbackColors.end()?it->second:PlaceholderColor{200,200,200};}
size_t AssetManager::preloadTextures(IRenderer& renderer){if(!m_hasManifest)return 0;size_t loaded=0;for(const auto& pair:m_manifest.textures)if(renderer.loadTexture(pair.first,m_dataDir+"/"+pair.second))++loaded;return loaded;}
std::string AssetManager::getBloonAssetId(BloonType type){switch(type){case BloonType::Red:return "bloon_red";case BloonType::Blue:return "bloon_blue";case BloonType::Green:return "bloon_green";case BloonType::Yellow:return "bloon_yellow";case BloonType::Pink:return "bloon_pink";case BloonType::Black:return "bloon_black";case BloonType::White:return "bloon_white";case BloonType::Lead:return "bloon_lead";case BloonType::Rainbow:return "bloon_rainbow";case BloonType::Ceramic:return "bloon_ceramic";case BloonType::MOAB:return "bloon_moab";default:return "";}}
std::string AssetManager::getTowerAssetId(TowerType type){switch(type){case TowerType::DartMonkey:return "tower_dart_monkey";case TowerType::TackShooter:return "tower_tack_shooter";case TowerType::SniperMonkey:return "tower_sniper_monkey";case TowerType::BoomerangThrower:return "tower_boomerang";case TowerType::BombTower:return "tower_bomb_tower";case TowerType::SuperMonkey:return "tower_super_monkey";default:return "";}}
std::string AssetManager::getProjectileAssetId(ProjectileType type){switch(type){case ProjectileType::Dart:return "projectile_dart";case ProjectileType::Tack:return "projectile_tack";case ProjectileType::Bomb:return "projectile_bomb";case ProjectileType::Boomerang:return "projectile_boomerang";case ProjectileType::SniperShot:return "projectile_bullet";case ProjectileType::Laser:return "projectile_laser";case ProjectileType::Plasma:return "projectile_plasma";default:return "";}}
void AssetManager::drawBloon(IRenderer& renderer,const Bloon& bloon) const{if(!bloon.active)return;const std::string logical=getBloonAssetId(bloon.type);const std::string id=findImportedTextureId(m_manifest,renderer,logical);if(!id.empty())renderer.drawSprite(id,bloon.x-bloon.radius,bloon.y-bloon.radius,bloon.radius*2.0f,bloon.radius*2.0f);else{PlaceholderColor pc=getPlaceholderColor(logical);Color c{pc.r,pc.g,pc.b,255};renderer.drawCircle(bloon.x,bloon.y-1.0f,bloon.radius,c,true);renderer.drawCircle(bloon.x,bloon.y-1.0f,bloon.radius,Color::black(),false);renderer.drawCircle(bloon.x-bloon.radius*.35f,bloon.y-bloon.radius*.45f,1.5f,Color::white(),true);renderer.drawRect(bloon.x-1.0f,bloon.y+bloon.radius-1.0f,2.0f,2.0f,c,true);}}
void AssetManager::drawTower(IRenderer& renderer,const Tower& tower,bool isSelected) const{if(isSelected)DebugRenderer::drawTowerRange(renderer,tower.x(),tower.y(),tower.range());const std::string logical=getTowerAssetId(tower.type());const std::string id=findImportedTextureId(m_manifest,renderer,logical);if(!id.empty())renderer.drawSprite(id,tower.x()-16.0f,tower.y()-16.0f,32.0f,32.0f);else{PlaceholderColor pc=getPlaceholderColor(logical);Color c{pc.r,pc.g,pc.b,255};renderer.drawCircle(tower.x(),tower.y(),12.0f,c,true);renderer.drawCircle(tower.x(),tower.y(),12.0f,Color::black(),false);renderer.drawCircle(tower.x(),tower.y(),4.0f,Color::white(),true);renderer.drawCircle(tower.x(),tower.y(),2.0f,Color::black(),true);}}
void AssetManager::drawProjectile(IRenderer& renderer,const Projectile& proj) const{if(!proj.active)return;const std::string logical=getProjectileAssetId(proj.type);const std::string id=findImportedTextureId(m_manifest,renderer,logical);if(!id.empty())renderer.drawSprite(id,proj.x-4.0f,proj.y-4.0f,8.0f,8.0f);else if(proj.type==ProjectileType::Bomb){renderer.drawCircle(proj.x,proj.y,4.0f,Color::black(),true);renderer.drawCircle(proj.x,proj.y,4.0f,Color::red(),false);}else if(proj.type==ProjectileType::Plasma)renderer.drawCircle(proj.x,proj.y,5.0f,Color::cyan(),true);else{float len=6.0f,speed=std::sqrt(proj.vx*proj.vx+proj.vy*proj.vy);float dx=speed>.001f?proj.vx/speed:1.0f,dy=speed>.001f?proj.vy/speed:0.0f;renderer.drawLine(proj.x,proj.y,proj.x-dx*len,proj.y-dy*len,Color::yellow());}}
void AssetManager::drawMap(IRenderer& renderer,const Map& map) const{
    std::string backgroundId;
    const std::string mapName = lowerId(map.name());

    // Prefer an explicitly matching HD map background. The imported definitive
    // package contains the real HD map art, so do not accidentally pick the
    // first alphabetically-sorted map texture (which used to make every map
    // render as Ant Hill).
    std::vector<std::string> candidates;
    if (mapName.find("farm") != std::string::npos) candidates.push_back("farm_yard");
    if (mapName.find("ocean") != std::string::npos) candidates.push_back("ocean_road");
    if (mapName.find("rail") != std::string::npos) candidates.push_back("rail_track");
    if (mapName.find("river") != std::string::npos) candidates.push_back("river_bed");
    if (mapName.find("snow") != std::string::npos) candidates.push_back("snow_trail");
    if (mapName.find("lava") != std::string::npos) candidates.push_back("lava_lake");
    if (mapName.find("pool") != std::string::npos) candidates.push_back("pool_party");
    if (mapName.find("bee") != std::string::npos) candidates.push_back("bee_hive");
    if (mapName.find("cactus") != std::string::npos) candidates.push_back("cactus_creek");
    if (mapName.find("daisy") != std::string::npos) candidates.push_back("daisy_chain");
    if (mapName.find("ant") != std::string::npos) candidates.push_back("ant_hill");
    if (mapName.find("world") != std::string::npos) candidates.push_back("world_tour");

    for (const auto& base : candidates) {
        const std::string hdHigh = base + "_high_res@hd";
        const std::string hd = base + "@hd";
        const std::string phoneHigh = base + "_high_res@phone";
        const std::string phone = base + "@phone";
        if (renderer.hasTexture(hdHigh)) { backgroundId = hdHigh; break; }
        if (renderer.hasTexture(hd)) { backgroundId = hd; break; }
        if (renderer.hasTexture(phoneHigh)) { backgroundId = phoneHigh; break; }
        if (renderer.hasTexture(phone)) { backgroundId = phone; break; }
    }

    // A deterministic desktop fallback for the built-in Classic Track.
    if (backgroundId.empty()) {
        const std::string fallbackIds[] = {
            "farm_yard_high_res@hd", "farm_yard@hd",
            "farm_yard_high_res@phone", "farm_yard@phone"
        };
        for (const auto& id : fallbackIds) {
            if (renderer.hasTexture(id)) { backgroundId = id; break; }
        }
    }

    if (!backgroundId.empty()) {
        renderer.drawSprite(backgroundId,0,0,480,272);
        return;
    }

    renderer.drawRect(0,0,480,272,{34,139,34,255},true);
    for (const auto& br:map.blockedRegions()){
        renderer.drawRect(br.x,br.y,br.w,br.h,{46,117,46,255},true);
        renderer.drawRect(br.x,br.y,br.w,br.h,{25,80,25,255},false);
    }
    for (const auto& path:map.paths()){
        const auto& w=path.waypoints();
        for(size_t i=0;i+1<w.size();++i){
            for(float o=-8;o<=8;o+=2) renderer.drawLine(w[i].x+o,w[i].y,w[i+1].x+o,w[i+1].y,{210,180,140,255});
            renderer.drawLine(w[i].x-9,w[i].y,w[i+1].x-9,w[i+1].y,{160,130,95,255});
            renderer.drawLine(w[i].x+9,w[i].y,w[i+1].x+9,w[i+1].y,{160,130,95,255});
        }
    }
}
void AssetManager::drawMainMenu(IRenderer& renderer, float pointerX, float pointerY) const {
    // The definitive/mobile package contains real BTD4 menu artwork. Prefer the
    // HD version for desktop and gracefully fall back to the phone/placeholder
    // presentation when a build was made without the optional front-end assets.
    const std::string backgroundCandidates[] = {
        "main_menu_high_res@hd",
        "main_menu@hd",
        "main_menu_high_res@phone",
        "main_menu@phone"
    };

    std::string backgroundId;
    for (const auto& id : backgroundCandidates) {
        if (renderer.hasTexture(id)) {
            backgroundId = id;
            break;
        }
    }

    if (!backgroundId.empty()) {
        renderer.drawSprite(backgroundId, 0.0f, 0.0f, 480.0f, 272.0f);
    } else {
        renderer.drawRect(0.0f, 0.0f, 480.0f, 272.0f, {42, 92, 50, 255}, true);
        renderer.drawRect(0.0f, 0.0f, 480.0f, 272.0f, {18, 42, 24, 255}, false);
        renderer.drawText("BLOONS TD 4", 145.0f, 36.0f, 2.2f, Color::white());
        renderer.drawText("REPPOPPED", 175.0f, 66.0f, 1.2f, {255, 220, 90, 255});
    }

    // Desktop presentation: keep the actual menu artwork visible and put the
    // controls in a compact glass-style panel rather than covering the scene.
    constexpr float panelX = 300.0f;
    constexpr float panelY = 102.0f;
    constexpr float panelW = 166.0f;
    constexpr float buttonX = 312.0f;
    constexpr float buttonW = 142.0f;
    constexpr float buttonH = 30.0f;

    renderer.drawRect(panelX, panelY, panelW, 148.0f, {8, 18, 12, 205}, true);
    renderer.drawRect(panelX, panelY, panelW, 148.0f, {112, 190, 120, 220}, false);
    renderer.drawText("DESKTOP", panelX + 16.0f, panelY + 10.0f, 0.9f, {185, 235, 190, 255});
    renderer.drawText("BLOONS TD 4", panelX + 16.0f, panelY + 27.0f, 1.15f, Color::white());

    const bool playHot = pointerX >= buttonX && pointerX <= buttonX + buttonW &&
                         pointerY >= 145.0f && pointerY <= 175.0f;
    const bool editorHot = pointerX >= buttonX && pointerX <= buttonX + buttonW &&
                           pointerY >= 182.0f && pointerY <= 212.0f;
    const bool exitHot = pointerX >= buttonX && pointerX <= buttonX + buttonW &&
                         pointerY >= 219.0f && pointerY <= 249.0f;

    const auto drawButton = [&renderer, buttonX, buttonW, buttonH](float y, const char* label, bool hot, bool destructive) {
        const Color fill = hot
            ? (destructive ? Color{145, 58, 58, 235} : Color{65, 135, 85, 245})
            : (destructive ? Color{82, 40, 40, 225} : Color{30, 70, 42, 235});
        const Color outline = hot
            ? Color::white()
            : (destructive ? Color{170, 90, 90, 230} : Color{105, 165, 115, 230});
        renderer.drawRect(buttonX, y, buttonW, buttonH, fill, true);
        renderer.drawRect(buttonX, y, buttonW, buttonH, outline, false);
        renderer.drawText(label, buttonX + 19.0f, y + 9.0f, 0.95f, Color::white());
    };

    drawButton(145.0f, "PLAY GAME", playHot, false);
    drawButton(182.0f, "TRACK EDITOR", editorHot, false);
    drawButton(219.0f, "EXIT", exitHot, true);

    renderer.drawText("Mouse + keyboard", panelX + 17.0f, 257.0f, 0.68f, {175, 190, 180, 255});
}

void AssetManager::drawHUD(IRenderer& renderer,const Economy& economy,int currentRound,size_t totalRounds,double fps,TowerType selectedPlacementType,bool hasPlacement) const{
    // Desktop/HD-style gameplay HUD. The renderer can scale this existing
    // world-space layout to any native desktop resolution, while PSP/Xbox keep
    // their own frontend profiles and controls.
    constexpr float panelX = 344.0f;
    constexpr float panelW = 136.0f;

    renderer.drawRect(0.0f, 0.0f, 480.0f, 30.0f, {8, 20, 12, 238}, true);
    renderer.drawRect(0.0f, 29.0f, 480.0f, 1.0f, {120, 190, 125, 255}, true);

    renderer.drawText("LIVES", 10.0f, 7.0f, 0.9f, {190, 225, 195, 255});
    renderer.drawText(std::to_string(economy.lives()), 48.0f, 6.0f, 1.35f, Color::red());
    renderer.drawText("CASH", 92.0f, 7.0f, 0.9f, {190, 225, 195, 255});
    renderer.drawText("$" + std::to_string(economy.cash()), 130.0f, 6.0f, 1.25f, {255, 225, 85, 255});
    renderer.drawText("ROUND", 222.0f, 7.0f, 0.9f, {190, 225, 195, 255});
    renderer.drawText(std::to_string(currentRound) + "/" + std::to_string(totalRounds), 270.0f, 6.0f, 1.15f, Color::white());

    const float progress = totalRounds > 0
        ? std::clamp(static_cast<float>(currentRound) / static_cast<float>(totalRounds), 0.0f, 1.0f)
        : 0.0f;
    renderer.drawRect(222.0f, 23.0f, 105.0f, 3.0f, {40, 70, 45, 255}, true);
    if (progress > 0.0f)
        renderer.drawRect(222.0f, 23.0f, 105.0f * progress, 3.0f, {120, 210, 130, 255}, true);

    renderer.drawRect(panelX, 30.0f, panelW, 242.0f, {8, 18, 12, 244}, true);
    renderer.drawRect(panelX, 30.0f, 1.0f, 242.0f, {110, 180, 120, 240}, true);
    renderer.drawText("TOWERS", panelX + 12.0f, 36.0f, 1.0f, Color::white());

    static const TowerType towers[] = {
        TowerType::DartMonkey,
        TowerType::TackShooter,
        TowerType::BombTower,
        TowerType::BoomerangThrower,
        TowerType::SuperMonkey
    };

    const auto towerName = [](TowerType tt) -> const char* {
        switch (tt) {
            case TowerType::DartMonkey: return "DART MONKEY";
            case TowerType::TackShooter: return "TACK SHOOTER";
            case TowerType::BombTower: return "BOMB TOWER";
            case TowerType::BoomerangThrower: return "BOOMERANG";
            case TowerType::SuperMonkey: return "SUPER MONKEY";
            default: return "TOWER";
        }
    };

    float y = 51.0f;
    for (TowerType tt : towers) {
        const auto stats = getTowerBaseStats(tt);
        const bool selected = hasPlacement && selectedPlacementType == tt;
        const bool affordable = economy.canAfford(stats.cost);
        const Color fill = selected
            ? Color{54, 112, 76, 250}
            : (affordable ? Color{24, 53, 32, 240} : Color{22, 28, 24, 225});
        const Color outline = selected ? Color{190, 245, 180, 255} : Color{77, 120, 85, 235};

        renderer.drawRect(panelX + 7.0f, y, panelW - 14.0f, 31.0f, fill, true);
        renderer.drawRect(panelX + 7.0f, y, panelW - 14.0f, 31.0f, outline, false);

        const std::string logical = getTowerAssetId(tt);
        const std::string id = findImportedTextureId(m_manifest, renderer, logical);
        if (!id.empty())
            renderer.drawSprite(id, panelX + 10.0f, y + 2.0f, 27.0f, 27.0f);

        renderer.drawText(towerName(tt), panelX + 42.0f, y + 4.0f, 0.72f,
            affordable ? Color::white() : Color{115, 125, 118, 255});
        renderer.drawText("$" + std::to_string(stats.cost), panelX + 42.0f, y + 18.0f, 0.85f,
            affordable ? Color{255, 225, 90, 255} : Color{185, 95, 95, 255});
        y += 34.0f;
    }

    // A real round-control cluster instead of the old debug "START ROUND"
    // rectangle. It remains intentionally mouse-friendly at desktop sizes.
    const bool canStart = !hasPlacement;
    const Color startFill = canStart ? Color{43, 103, 58, 245} : Color{37, 48, 40, 220};
    renderer.drawRect(panelX + 7.0f, 224.0f, panelW - 14.0f, 20.0f, startFill, true);
    renderer.drawRect(panelX + 7.0f, 224.0f, panelW - 14.0f, 20.0f, {120, 195, 125, 230}, false);
    renderer.drawText("NEXT ROUND", panelX + 29.0f, 230.0f, 0.78f, Color::white());

    renderer.drawRect(panelX + 7.0f, 247.0f, 61.0f, 20.0f, {40, 66, 48, 235}, true);
    renderer.drawRect(panelX + 7.0f, 247.0f, 61.0f, 20.0f, {100, 150, 105, 220}, false);
    renderer.drawText("EDIT", panelX + 27.0f, 253.0f, 0.78f, Color::white());

    renderer.drawRect(panelX + 73.0f, 247.0f, 57.0f, 20.0f, {70, 36, 36, 235}, true);
    renderer.drawRect(panelX + 73.0f, 247.0f, 57.0f, 20.0f, {185, 92, 92, 230}, false);
    renderer.drawText("SELL", panelX + 89.0f, 253.0f, 0.78f, Color::white());

    (void)fps; // FPS is deliberately no longer presented as debug UI.
}


} // namespace btd4
