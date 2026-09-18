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
void AssetManager::drawHUD(IRenderer& renderer,const Economy& economy,int currentRound,size_t totalRounds,double fps,TowerType selectedPlacementType,bool hasPlacement) const{
    // Desktop/HD-style HUD laid out in the existing 480x272 world space. The
    // renderer now scales this layout to the actual desktop viewport, so it is
    // no longer rasterized at PSP resolution.
    renderer.drawRect(0,0,480,30,{18,24,30,245},true);
    renderer.drawRect(0,29,480,1,{70,85,95,255},true);
    renderer.drawText("LIVES",10,7,1.0f,{180,220,235,255});
    renderer.drawText(std::to_string(economy.lives()),50,7,1.2f,Color::red());
    renderer.drawText("CASH",92,7,1.0f,{180,220,235,255});
    renderer.drawText("$"+std::to_string(economy.cash()),130,7,1.2f,Color::yellow());
    renderer.drawText("ROUND",220,7,1.0f,{180,220,235,255});
    renderer.drawText(std::to_string(currentRound)+"/"+std::to_string(totalRounds),265,7,1.2f,Color::white());
    renderer.drawText("FPS "+std::to_string((int)std::round(fps)),385,7,1.0f,{150,170,180,255});

    constexpr float panelX=368.0f;
    renderer.drawRect(panelX,30,112,242,{24,30,36,250},true);
    renderer.drawRect(panelX,30,1,242,{75,90,100,255},true);
    renderer.drawText("TOWERS",378,36,1.0f,Color::white());

    static const TowerType towers[]={TowerType::DartMonkey,TowerType::TackShooter,TowerType::BombTower,TowerType::BoomerangThrower,TowerType::SuperMonkey};
    float y=50.0f;
    for(TowerType tt:towers){
        auto stats=getTowerBaseStats(tt);
        bool sel=hasPlacement&&selectedPlacementType==tt;
        bool afford=economy.canAfford(stats.cost);
        Color bg=sel?Color{55,95,135,255}:(afford?Color{39,47,53,255}:Color{28,32,36,255});
        renderer.drawRect(panelX+6,y,100,32,bg,true);
        renderer.drawRect(panelX+6,y,100,32,sel?Color::cyan():Color{70,78,84,255},false);
        const std::string logical=getTowerAssetId(tt);
        const std::string id=findImportedTextureId(m_manifest,renderer,logical);
        if(!id.empty()) renderer.drawSprite(id,panelX+10,y+2,28,28);
        std::string n;
        switch(tt){
            case TowerType::DartMonkey:n="DART MONKEY";break;
            case TowerType::TackShooter:n="TACK SHOOTER";break;
            case TowerType::BombTower:n="BOMB TOWER";break;
            case TowerType::BoomerangThrower:n="BOOMERANG";break;
            case TowerType::SuperMonkey:n="SUPER MONKEY";break;
            default:n="TOWER";break;
        }
        renderer.drawText(n,panelX+41,y+5,0.8f,afford?Color::white():Color{115,120,125,255});
        renderer.drawText("$"+std::to_string(stats.cost),panelX+41,y+18,0.9f,afford?Color::yellow():Color::red());
        y+=35.0f;
    }

    renderer.drawRect(panelX+6,224,100,20,{42,50,56,255},true);
    renderer.drawRect(panelX+6,224,100,20,Color::cyan(),false);
    renderer.drawText("TARGET",panelX+29,230,0.9f,Color::white());
    renderer.drawRect(panelX+6,247,100,20,{65,42,42,255},true);
    renderer.drawRect(panelX+6,247,100,20,Color::red(),false);
    renderer.drawText("SELL",panelX+42,253,0.9f,Color::white());

    renderer.drawRect(8,236,150,30,{36,82,45,255},true);
    renderer.drawRect(8,236,150,30,Color::green(),false);
    renderer.drawText("START ROUND",24,245,1.0f,Color::white());
}

} // namespace btd4
