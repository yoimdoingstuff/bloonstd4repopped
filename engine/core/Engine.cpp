#include "Engine.hpp"
#include "../rendering/DebugRenderer.hpp"
#include "../../platform/common/NativeFileSystem.hpp"
#include "../map/MapLoader.hpp"
#include "../game/TowerData.hpp"
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <utility>

namespace btd4 {

namespace {
TowerType towerForAction(InputAction action, bool& matched) {
    matched = true;
    switch (action) {
        case InputAction::SelectTower1: return TowerType::DartMonkey;
        case InputAction::SelectTower2: return TowerType::TackShooter;
        case InputAction::SelectTower3: return TowerType::BombTower;
        case InputAction::SelectTower4: return TowerType::BoomerangThrower;
        case InputAction::SelectTower5: return TowerType::SuperMonkey;
        default: matched = false; return TowerType::DartMonkey;
    }
}

std::string findRuntimeDataDirectory(const std::string& root) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path direct(root);
    if (fs::is_regular_file(direct / "manifest.json", ec)) return direct.string();
    if (!fs::is_directory(direct, ec)) return {};

    std::vector<fs::path> candidates;
    for (const auto& entry : fs::directory_iterator(direct, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) break;
        if (!entry.is_directory(ec)) continue;
        if (fs::is_regular_file(entry.path() / "manifest.json", ec)) candidates.push_back(entry.path());
    }
    std::sort(candidates.begin(), candidates.end());
    return candidates.size() == 1 ? candidates.front().string() : std::string{};
}
}

Engine::Engine(IRenderer& renderer, IInput& input, FrontendProfile frontendProfile)
    : m_renderer(renderer), m_input(input), m_frontendProfile(frontendProfile) {}

Engine::~Engine() { shutdown(); }

bool Engine::initialize(int windowWidth, int windowHeight) {
    BTD4_LOG_INFO(m_frontendProfile == FrontendProfile::PspConsole
        ? "Initializing BTD4 Engine (PSP 480x272 frontend)..."
        : "Initializing BTD4 Engine (resizable desktop frontend)...");
    if (!m_renderer.initialize(windowWidth, windowHeight)) {
        BTD4_LOG_ERROR("Failed to initialize renderer!");
        return false;
    }
    onResize(windowWidth, windowHeight);
    m_clock.reset();
    m_running = true;
    m_desktopScreen = (m_frontendProfile == FrontendProfile::FlashDesktop)
        ? DesktopScreen::MainMenu
        : DesktopScreen::Gameplay;

    NativeFileSystem fs;
    const std::string dataDir = "game_data";
    AssetManager::instance().initialize(fs, dataDir);
    size_t preloaded = AssetManager::instance().preloadTextures(m_renderer);
    const std::string runtimeDataDir = AssetManager::instance().dataDirectory();
    BTD4_LOG_INFO("Asset pipeline initialized. Runtime data directory: " + runtimeDataDir);
    BTD4_LOG_INFO("Textures preloaded into renderer: " + std::to_string(preloaded));

    Map gameMap("Classic Track");
    bool loadedImportedMap = false;
    std::string mapErr;
    const auto& manifest = AssetManager::instance().manifest();
    if (!runtimeDataDir.empty() && !manifest.maps.empty()) {
        for (const std::string& mapPath : manifest.maps) {
            Map candidate;
            if (loadMap(fs, runtimeDataDir + "/" + mapPath, candidate, mapErr)) {
                gameMap = std::move(candidate);
                loadedImportedMap = true;
                BTD4_LOG_INFO("Loaded imported map: " + mapPath);
                break;
            }
        }
        if (!loadedImportedMap && !mapErr.empty()) {
            BTD4_LOG_WARN("Imported map data was listed but could not be loaded: " + mapErr);
        }
    }

    if (!loadedImportedMap && !runtimeDataDir.empty()) {
        namespace stdfs = std::filesystem;
        std::error_code mapEc;
        const stdfs::path customMapDir = stdfs::path(runtimeDataDir) / "maps";
        if (stdfs::is_directory(customMapDir, mapEc)) {
            std::vector<stdfs::path> customMaps;
            for (stdfs::directory_iterator it(customMapDir, stdfs::directory_options::skip_permission_denied, mapEc), end;
                 it != end && !mapEc; it.increment(mapEc)) {
                if (it->is_regular_file(mapEc) && it->path().extension() == ".json") {
                    customMaps.push_back(it->path());
                }
            }
            std::sort(customMaps.begin(), customMaps.end());
            for (const auto& mapPath : customMaps) {
                Map candidate;
                if (loadMap(fs, mapPath.string(), candidate, mapErr)) {
                    gameMap = std::move(candidate);
                    loadedImportedMap = true;
                    BTD4_LOG_INFO("Loaded custom packaged map: " + mapPath.string());
                    break;
                }
            }
        }
    }

    if (!loadedImportedMap) {
        gameMap.setName("Classic Track");
        gameMap.addPath(Path({{-20.0f, 136.0f}, {100.0f, 136.0f}, {100.0f, 60.0f},
                              {220.0f, 60.0f}, {220.0f, 210.0f}, {340.0f, 210.0f},
                              {340.0f, 136.0f}, {500.0f, 136.0f}}));
        gameMap.addBuildableRegion({0.0f, 0.0f, 400.0f, 272.0f});
        gameMap.addBlockedRegion({80.0f, 40.0f, 40.0f, 120.0f});
        gameMap.addBlockedRegion({200.0f, 40.0f, 40.0f, 190.0f});
        gameMap.addBlockedRegion({320.0f, 116.0f, 40.0f, 114.0f});
        BTD4_LOG_INFO("No usable imported map found; using built-in fallback map geometry.");
    }
    m_simulation.setMap(std::move(gameMap));
    m_simulation.setState(GameStateType::Playing);

    std::string towerErr;
    const std::string customTowers = runtimeDataDir.empty()
        ? std::string{}
        : runtimeDataDir + "/towers/custom_towers.json";
    const std::string importedTowers = runtimeDataDir.empty()
        ? std::string{}
        : runtimeDataDir + "/towers/default_towers.json";
    const std::string placeholderTowers = "assets/placeholder/towers/default_towers.json";
    TowerSet towerSet;
    bool loadedCustomTowers = !customTowers.empty() &&
        loadTowers(fs, customTowers, towerSet, towerErr);
    bool loadedImportedTowers = !loadedCustomTowers && !importedTowers.empty() &&
        fs.fileExists(importedTowers) && loadTowers(fs, importedTowers, towerSet, towerErr);
    if (loadedCustomTowers || loadedImportedTowers ||
        loadTowers(fs, placeholderTowers, towerSet, towerErr)) {
        configureTowerDefinitions(towerSet);
        BTD4_LOG_INFO(std::string("Loaded ") +
            (loadedCustomTowers ? "custom" : (loadedImportedTowers ? "imported" : "fallback")) +
            " tower definitions (" + std::to_string(towerSet.towers.size()) + ").");
    } else {
        BTD4_LOG_WARN("Tower data unavailable: " + towerErr + "; built-in stats remain active.");
    }

    std::string roundErr;
    RoundSet roundSet;
    const std::string customRounds = runtimeDataDir.empty()
        ? std::string{}
        : runtimeDataDir + "/rounds/custom_rounds.json";
    const std::string manifestRounds = manifest.roundsFile.empty()
        ? std::string{}
        : runtimeDataDir + "/" + manifest.roundsFile;
    const std::string placeholderRounds = "assets/placeholder/rounds/default_rounds.json";
    bool loadedCustomRounds = !customRounds.empty() &&
        loadRounds(fs, customRounds, m_simulation.map(), roundSet, roundErr);
    bool loadedImportedRounds = !loadedCustomRounds && !manifestRounds.empty() &&
        loadRounds(fs, manifestRounds, m_simulation.map(), roundSet, roundErr);

    if (loadedCustomRounds || loadedImportedRounds ||
        loadRounds(fs, placeholderRounds, m_simulation.map(), roundSet, roundErr)) {
        if (!m_simulation.setRounds(std::move(roundSet), roundErr)) {
            BTD4_LOG_WARN("Round data loaded but could not be configured: " + roundErr);
        } else {
            const char* source = loadedCustomRounds ? "custom"
                : (loadedImportedRounds ? "imported" : "fallback");
            BTD4_LOG_INFO(std::string("Loaded ") + source +
                " BTD4 round data successfully.");
        }
    } else {
        BTD4_LOG_WARN("Round data unavailable: " + roundErr);
    }

    std::string upgradeErr;
    const std::string importedUpgrades = runtimeDataDir.empty()
        ? std::string{}
        : runtimeDataDir + "/upgrades/default_upgrades.json";
    const std::string customUpgrades = runtimeDataDir.empty()
        ? std::string{}
        : runtimeDataDir + "/upgrades/custom_upgrades.json";
    const std::string placeholderUpgrades = "assets/placeholder/upgrades/default_upgrades.json";
    bool loadedCustomUpgrades = !customUpgrades.empty() &&
        fs.fileExists(customUpgrades) &&
        loadUpgrades(fs, customUpgrades, m_upgrades, upgradeErr);
    bool loadedImportedUpgrades = !loadedCustomUpgrades && !importedUpgrades.empty() &&
        loadUpgrades(fs, importedUpgrades, m_upgrades, upgradeErr);
    if (loadedCustomUpgrades || loadedImportedUpgrades ||
        loadUpgrades(fs, placeholderUpgrades, m_upgrades, upgradeErr)) {
        BTD4_LOG_INFO("Loaded " +
            std::string(loadedCustomUpgrades ? "custom" :
                (loadedImportedUpgrades ? "imported" : "fallback")) +
            " upgrade definitions (" + std::to_string(m_upgrades.upgrades.size()) + ").");
    } else {
        m_upgrades.upgrades.clear();
        BTD4_LOG_WARN("Upgrade data unavailable: " + upgradeErr);
    }

    switch (m_frontendProfile) {
        case FrontendProfile::FlashDesktop: BTD4_LOG_INFO("Frontend: Flash desktop mouse-first controls."); break;
        case FrontendProfile::PspConsole: BTD4_LOG_INFO("Frontend: PSP controls with D-pad/analog virtual cursor."); break;
        case FrontendProfile::XboxConsole: BTD4_LOG_INFO("Frontend: Xbox console controls with gamepad virtual cursor."); break;
    }
    BTD4_LOG_INFO("BTD4 Engine initialized successfully.");
    return true;
}

void Engine::shutdown() {
    if (m_running) {
        BTD4_LOG_INFO("Shutting down BTD4 Engine...");
        m_renderer.shutdown();
        m_running = false;
        BTD4_LOG_INFO("BTD4 Engine shutdown complete.");
    }
}

void Engine::onResize(int windowWidth, int windowHeight) {
    m_viewport = LogicalResolution::calculateViewport(windowWidth, windowHeight, m_frontendProfile == FrontendProfile::PspConsole);
    m_renderer.onResize(windowWidth, windowHeight);
}

void Engine::startNextRound() { if (!m_simulation.roundActive()) m_simulation.startNextRound(); }
void Engine::selectTowerType(TowerType type) { m_hasPlacement = true; m_placementType = type; }
void Engine::cancelPlacement() { m_hasPlacement = false; }

bool Engine::applySelectedUpgrade(uint8_t path) {
    Tower* tower = m_simulation.findTower(m_selectedTowerId);
    if (!tower) {
        BTD4_LOG_WARN("Upgrade requested without a selected tower.");
        return false;
    }
    const uint8_t tier = static_cast<uint8_t>(tower->upgradeTier(path) + 1);
    const UpgradeDefinition* definition = findUpgrade(m_upgrades, tower->type(), path, tier);
    if (!definition) {
        BTD4_LOG_INFO("No further upgrades are defined for this path.");
        return false;
    }
    if (!m_simulation.economy().spendCash(definition->effect.cost)) {
        BTD4_LOG_INFO("Cannot afford upgrade: " + definition->displayName);
        return false;
    }
    if (!tower->applyUpgrade(definition->effect, path, tier)) {
        m_simulation.economy().addCash(definition->effect.cost);
        BTD4_LOG_WARN("Upgrade application failed; purchase was refunded.");
        return false;
    }
    BTD4_LOG_INFO("Purchased " + definition->displayName + " for $" + std::to_string(definition->effect.cost) + ".");
    return true;
}

void Engine::frame(int windowWidth, int windowHeight) {
    (void)windowWidth; (void)windowHeight;
    if (!m_running) return;
    m_clock.tick();

    PointerState ptr = m_input.pointerState();
    m_testScreen.setCursorPosition(ptr.logicalX, ptr.logicalY);

    if (m_input.isActionJustPressed(InputAction::OpenTrackEditor)) {
        if (!m_simulation.roundActive()) {
            m_trackEditor.open(m_simulation.map());
        } else {
            BTD4_LOG_INFO("Track Editor can only be opened between rounds.");
        }
    }

    if (m_trackEditor.isOpen()) {
        m_trackEditor.update(m_input, ptr);
        Map editedMap;
        if (m_trackEditor.consumeApplyRequest(editedMap)) {
            m_simulation.reset();
            m_simulation.setMap(std::move(editedMap));
            m_simulation.setState(GameStateType::Playing);
            m_selectedTowerId = 0;
            m_hasPlacement = false;
            BTD4_LOG_INFO("Track Editor play test applied to the active game map.");
        }

        m_renderer.beginFrame();
        m_renderer.clear(Color::black());
        m_renderer.setViewport(m_viewport);
        m_trackEditor.render(m_renderer);
        m_renderer.endFrame();
        return;
    }
    // Desktop gets a genuine front-end state instead of booting directly into
    // gameplay. Console profiles intentionally keep their existing direct-to-game
    // flow so the PSP/Xbox presentation remains distinct.
    if (m_frontendProfile == FrontendProfile::FlashDesktop &&
        m_desktopScreen == DesktopScreen::MainMenu) {
        if (m_input.isActionJustPressed(InputAction::Cancel)) {
            requestExit();
        } else if (m_input.isActionJustPressed(InputAction::Confirm)) {
            const bool play = ptr.logicalX >= 312.0f && ptr.logicalX <= 454.0f &&
                              ptr.logicalY >= 145.0f && ptr.logicalY <= 175.0f;
            const bool editor = ptr.logicalX >= 312.0f && ptr.logicalX <= 454.0f &&
                                ptr.logicalY >= 182.0f && ptr.logicalY <= 212.0f;
            const bool exit = ptr.logicalX >= 312.0f && ptr.logicalX <= 454.0f &&
                              ptr.logicalY >= 219.0f && ptr.logicalY <= 249.0f;
            if (play) {
                m_desktopScreen = DesktopScreen::Gameplay;
                m_simulation.setState(GameStateType::Playing);
            } else if (editor) {
                m_trackEditor.open(m_simulation.map());
            } else if (exit) {
                requestExit();
            }
        } else if (m_input.isActionJustPressed(InputAction::StartRound)) {
            m_desktopScreen = DesktopScreen::Gameplay;
            m_simulation.setState(GameStateType::Playing);
        }

        if (m_trackEditor.isOpen()) {
            m_trackEditor.update(m_input, ptr);
            Map editedMap;
            if (m_trackEditor.consumeApplyRequest(editedMap)) {
                m_simulation.reset();
                m_simulation.setMap(std::move(editedMap));
                m_simulation.setState(GameStateType::Playing);
                m_selectedTowerId = 0;
                m_hasPlacement = false;
                BTD4_LOG_INFO("Track Editor play test applied to the active game map.");
            }

            m_renderer.beginFrame();
            m_renderer.clear(Color::black());
            m_renderer.setViewport(m_viewport);
            m_trackEditor.render(m_renderer);
            m_renderer.endFrame();
            return;
        }

        m_renderer.beginFrame();
        m_renderer.clear(Color::black());
        m_renderer.setViewport(m_viewport);
        AssetManager::instance().drawMainMenu(m_renderer, ptr.logicalX, ptr.logicalY);
        m_renderer.endFrame();
        return;
    }

    const InputAction towerActions[] = {InputAction::SelectTower1, InputAction::SelectTower2,
        InputAction::SelectTower3, InputAction::SelectTower4, InputAction::SelectTower5};
    for (const InputAction action : towerActions) {
        if (m_input.isActionJustPressed(action)) {
            bool matched = false;
            const TowerType type = towerForAction(action, matched);
            if (matched) { selectTowerType(type); break; }
        }
    }
    if (m_input.isActionJustPressed(InputAction::StartRound)) startNextRound();

    if (m_input.isActionJustPressed(InputAction::Confirm)) {
        // Desktop HUD layout:
        //   x 344..480, y 51..221 -> tower shop
        //   x 344..480, y 224..244 -> next round
        //   x 344..412, y 247..267 -> track editor
        //   x 417..480, y 247..267 -> sell
        if (ptr.logicalX >= 344.0f && ptr.logicalX <= 480.0f &&
            ptr.logicalY >= 51.0f && ptr.logicalY < 221.0f) {
            const int idx = static_cast<int>((ptr.logicalY - 51.0f) / 34.0f);
            if (idx >= 0 && idx < 5) {
                static const TowerType tts[] = {
                    TowerType::DartMonkey,
                    TowerType::TackShooter,
                    TowerType::BombTower,
                    TowerType::BoomerangThrower,
                    TowerType::SuperMonkey
                };
                selectTowerType(tts[idx]);
            }
        } else if (ptr.logicalX >= 344.0f && ptr.logicalX <= 480.0f &&
                   ptr.logicalY >= 224.0f && ptr.logicalY < 244.0f) {
            if (!m_simulation.roundActive()) startNextRound();
        } else if (ptr.logicalX >= 344.0f && ptr.logicalX < 412.0f &&
                   ptr.logicalY >= 247.0f && ptr.logicalY < 267.0f) {
            if (!m_simulation.roundActive()) {
                m_trackEditor.open(m_simulation.map());
            } else {
                BTD4_LOG_INFO("Track Editor can only be opened between rounds.");
            }
        } else if (ptr.logicalX >= 417.0f && ptr.logicalX <= 480.0f &&
                   ptr.logicalY >= 247.0f && ptr.logicalY < 267.0f) {
            if (m_selectedTowerId != 0 && m_simulation.sellTower(m_selectedTowerId)) {
                m_selectedTowerId = 0;
                m_hasPlacement = false;
            }
        } else if (m_selectedTowerId != 0 &&
                   ptr.logicalX >= 168.0f && ptr.logicalX < 362.0f &&
                   ptr.logicalY >= 236.0f && ptr.logicalY < 266.0f) {
            applySelectedUpgrade(ptr.logicalX < 265.0f ? 0 : 1);
        } else if (ptr.logicalX < 344.0f) {
            if (m_hasPlacement) {
                if (m_simulation.placeTower(m_placementType, ptr.logicalX, ptr.logicalY)) cancelPlacement();
            } else {
                m_selectedTowerId = 0;
                for (const auto& tower : m_simulation.towers()) {
                    const float dx = tower.x() - ptr.logicalX;
                    const float dy = tower.y() - ptr.logicalY;
                    if (dx * dx + dy * dy <= 16.0f * 16.0f) {
                        m_selectedTowerId = tower.id();
                        break;
                    }
                }
            }
        }
    }
    if (m_selectedTowerId != 0 && m_input.isActionJustPressed(InputAction::NextTarget)) {
        if (Tower* tower = m_simulation.findTower(m_selectedTowerId)) {
            tower->cycleTargetingMode();
            BTD4_LOG_INFO("Targeting mode advanced.");
        }
    }
    if (m_selectedTowerId != 0 && m_input.isActionJustPressed(InputAction::PrevTarget)) {
        if (Tower* tower = m_simulation.findTower(m_selectedTowerId)) {
            tower->cycleTargetingModeBackward();
            BTD4_LOG_INFO("Targeting mode reversed.");
        }
    }

    if (m_input.isActionJustPressed(InputAction::Upgrade)) applySelectedUpgrade(0);
    if (m_input.isActionJustPressed(InputAction::UpgradePath1)) applySelectedUpgrade(0);
    if (m_input.isActionJustPressed(InputAction::UpgradePath2)) applySelectedUpgrade(1);
    if (m_input.isActionJustPressed(InputAction::Sell) && m_selectedTowerId != 0) {
        if (m_simulation.sellTower(m_selectedTowerId)) {
            m_selectedTowerId = 0;
            m_hasPlacement = false;
        }
    }
    if (m_input.isActionJustPressed(InputAction::Cancel)) {
        if (m_frontendProfile == FrontendProfile::FlashDesktop &&
            !m_hasPlacement && m_selectedTowerId == 0) {
            m_desktopScreen = DesktopScreen::MainMenu;
            return;
        }
        cancelPlacement();
        m_selectedTowerId = 0;
    }
    if (m_input.isActionJustPressed(InputAction::Pause)) {
        if (m_simulation.state() == GameStateType::Paused) m_simulation.resume();
        else if (m_simulation.state() == GameStateType::Playing) m_simulation.pause();
    }

    while (m_clock.hasFixedTick()) {
        if (m_gameMode) m_simulation.update(static_cast<float>(m_clock.fixedTimeStep()));
        else m_testScreen.update(m_clock.fixedTimeStep());
        m_clock.consumeFixedTick();
    }

    m_renderer.beginFrame();
    m_renderer.clear(Color::black());
    m_renderer.setViewport(m_viewport);
    if (m_gameMode) {
        AssetManager::instance().drawMap(m_renderer, m_simulation.map());
        for (const auto& tower : m_simulation.towers()) AssetManager::instance().drawTower(m_renderer, tower, tower.id() == m_selectedTowerId);
        if (m_hasPlacement && ptr.logicalX < 400.0f) {
            auto stats = getTowerBaseStats(m_placementType);
            bool canPlace = m_simulation.map().canPlaceTower(ptr.logicalX, ptr.logicalY, stats.footprintRadius) && m_simulation.economy().canAfford(stats.cost);
            Color rangeColor = canPlace ? Color::cyan() : Color::red();
            DebugRenderer::drawTowerRange(m_renderer, ptr.logicalX, ptr.logicalY, stats.range, rangeColor, {rangeColor.r, rangeColor.g, rangeColor.b, 30});
            Tower previewTower(0, m_placementType, ptr.logicalX, ptr.logicalY);
            AssetManager::instance().drawTower(m_renderer, previewTower, false);
        }
        std::vector<const Bloon*> activeBloons;
        m_simulation.bloonPool().getActiveBloons(activeBloons);
        for (const auto* bloon : activeBloons) if (bloon) AssetManager::instance().drawBloon(m_renderer, *bloon);
        for (const auto& proj : m_simulation.projectilePool().allProjectiles()) if (proj.active) AssetManager::instance().drawProjectile(m_renderer, proj);
        AssetManager::instance().drawHUD(m_renderer, m_simulation.economy(), m_simulation.currentRound(),
            m_simulation.completedRounds() + (m_simulation.roundActive() ? 1 : 0), m_clock.fps(), m_placementType, m_hasPlacement);
        if (Tower* selectedTower = m_simulation.findTower(m_selectedTowerId)) {
            const auto towerLabel = [](TowerType type) {
                switch (type) {
                    case TowerType::DartMonkey: return std::string("DART MONKEY");
                    case TowerType::TackShooter: return std::string("TACK SHOOTER");
                    case TowerType::SniperMonkey: return std::string("SNIPER");
                    case TowerType::BoomerangThrower: return std::string("BOOMERANG");
                    case TowerType::BombTower: return std::string("BOMB TOWER");
                    case TowerType::SuperMonkey: return std::string("SUPER MONKEY");
                    default: return std::string("TOWER");
                }
            };

            // Keep selection information in the bottom command strip so the
            // playfield itself is not obscured by a debug-looking banner.
            m_renderer.drawRect(4.0f, 225.0f, 156.0f, 43.0f, {5, 18, 9, 235}, true);
            m_renderer.drawRect(4.0f, 225.0f, 156.0f, 43.0f, {105, 180, 115, 230}, false);

            m_renderer.drawText(towerLabel(selectedTower->type()), 12.0f, 230.0f, 0.82f, Color::white());

            const char* targetingLabel = selectedTower->targetingMode() == TargetingMode::First ? "FIRST"
                : (selectedTower->targetingMode() == TargetingMode::Last ? "LAST"
                : (selectedTower->targetingMode() == TargetingMode::Close ? "CLOSE" : "STRONG"));
            m_renderer.drawText("TARGET " + std::string(targetingLabel), 12.0f, 247.0f, 0.72f,
                {170, 225, 180, 255});

            for (uint8_t path = 0; path < 2; ++path) {
                const uint8_t nextTier = static_cast<uint8_t>(selectedTower->upgradeTier(path) + 1);
                const UpgradeDefinition* upgrade = findUpgrade(m_upgrades, selectedTower->type(), path, nextTier);
                const float boxX = path == 0 ? 168.0f : 265.0f;
                m_renderer.drawRect(boxX, 236.0f, 92.0f, 30.0f, {18, 40, 26, 245}, true);
                m_renderer.drawRect(boxX, 236.0f, 92.0f, 30.0f,
                    upgrade ? Color{105, 165, 115, 230} : Color{70, 80, 74, 220}, false);

                if (!upgrade) {
                    m_renderer.drawText("PATH " + std::to_string(path + 1) + " MAX",
                        boxX + 14.0f, 245.0f, 0.75f, {125, 135, 128, 255});
                } else {
                    const bool affordable = m_simulation.economy().canAfford(upgrade->effect.cost);
                    const Color valueColor = affordable
                        ? Color{255, 225, 90, 255}
                        : Color{185, 95, 95, 255};
                    m_renderer.drawText("PATH " + std::to_string(path + 1),
                        boxX + 8.0f, 240.0f, 0.67f, Color::white());
                    m_renderer.drawText(upgrade->displayName,
                        boxX + 8.0f, 250.0f, 0.59f, Color::white());
                    m_renderer.drawText("$" + std::to_string(upgrade->effect.cost),
                        boxX + 58.0f, 240.0f, 0.65f, valueColor);
                }
            }
        }
        if (m_frontendProfile != FrontendProfile::FlashDesktop) m_renderer.drawRect(ptr.logicalX - 4.0f, ptr.logicalY - 4.0f, 8.0f, 8.0f, Color::white(), false);
        if (m_simulation.state() == GameStateType::Paused) {
            m_renderer.drawRect(90.0f, 100.0f, 220.0f, 72.0f, {0, 0, 0, 210}, true);
            m_renderer.drawText("PAUSED", 170.0f, 118.0f, 2.0f, Color::white());
            if (m_frontendProfile == FrontendProfile::FlashDesktop) m_renderer.drawText("Click PAUSE or press P", 120.0f, 145.0f, 1.0f, Color::cyan());
            else if (m_frontendProfile == FrontendProfile::PspConsole) m_renderer.drawText("SELECT to resume", 143.0f, 145.0f, 1.0f, Color::cyan());
            else m_renderer.drawText("START to resume", 145.0f, 145.0f, 1.0f, Color::cyan());
        } else if (m_simulation.state() == GameStateType::GameOver) {
            m_renderer.drawRect(70.0f, 92.0f, 260.0f, 88.0f, {0, 0, 0, 220}, true);
            m_renderer.drawText("GAME OVER", 145.0f, 112.0f, 2.0f, Color::red());
            m_renderer.drawText("Press ESC to exit", 135.0f, 145.0f, 1.0f, Color::white());
        } else if (m_simulation.state() == GameStateType::Victory) {
            m_renderer.drawRect(70.0f, 92.0f, 260.0f, 88.0f, {0, 0, 0, 220}, true);
            m_renderer.drawText("VICTORY!", 145.0f, 112.0f, 2.0f, Color::green());
            m_renderer.drawText("All rounds complete", 125.0f, 145.0f, 1.0f, Color::white());
        }
    } else m_testScreen.render(m_renderer, m_clock.fps());
    m_renderer.endFrame();
}

bool Engine::isRunning() const { return m_running; }
void Engine::requestExit() { m_running = false; }
Clock& Engine::clock() { return m_clock; }
const Clock& Engine::clock() const { return m_clock; }
const Viewport& Engine::currentViewport() const { return m_viewport; }

} // namespace btd4
