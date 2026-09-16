#include "Engine.hpp"
#include "../rendering/DebugRenderer.hpp"
#include "../../platform/common/NativeFileSystem.hpp"
#include <cmath>

namespace btd4 {

Engine::Engine(IRenderer& renderer, IInput& input)
    : m_renderer(renderer), m_input(input) {
}

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize(int windowWidth, int windowHeight) {
    BTD4_LOG_INFO("Initializing BTD4 Engine (Logical Resolution: 480x272)...");

    if (!m_renderer.initialize(windowWidth, windowHeight)) {
        BTD4_LOG_ERROR("Failed to initialize renderer!");
        return false;
    }

    onResize(windowWidth, windowHeight);
    m_clock.reset();
    m_running = true;

    NativeFileSystem fs;
    AssetManager::instance().initialize(fs);
    size_t preloaded = AssetManager::instance().preloadTextures(m_renderer);
    BTD4_LOG_INFO("Asset pipeline initialized. Textures preloaded into renderer: " + std::to_string(preloaded));

    Map gameMap("Classic Track");
    gameMap.addPath(Path({
        {-20.0f, 136.0f},
        {100.0f, 136.0f},
        {100.0f, 60.0f},
        {220.0f, 60.0f},
        {220.0f, 210.0f},
        {340.0f, 210.0f},
        {340.0f, 136.0f},
        {500.0f, 136.0f}
    }));
    gameMap.addBuildableRegion({0.0f, 0.0f, 400.0f, 272.0f});
    gameMap.addBlockedRegion({80.0f, 40.0f, 40.0f, 120.0f});
    gameMap.addBlockedRegion({200.0f, 40.0f, 40.0f, 190.0f});
    gameMap.addBlockedRegion({320.0f, 116.0f, 40.0f, 114.0f});
    m_simulation.setMap(std::move(gameMap));
    m_simulation.setState(GameStateType::Playing);

    std::string roundErr;
    RoundSet roundSet;
    std::string roundPath = "assets/placeholder/rounds/default_rounds.json";
    if (loadRounds(fs, roundPath, m_simulation.map(), roundSet, roundErr)) {
        if (!m_simulation.setRounds(std::move(roundSet), roundErr)) {
            BTD4_LOG_WARN("Round data loaded but could not be configured: " + roundErr);
        } else {
            BTD4_LOG_INFO("Loaded round data successfully.");
        }
    } else {
        BTD4_LOG_WARN("Round data unavailable: " + roundErr);
    }

    BTD4_LOG_INFO("Gameplay controls: click a tower, click the map to place it, R starts the next round, P pauses, right-click cancels placement.");
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
    m_viewport = LogicalResolution::calculateViewport(windowWidth, windowHeight);
    m_renderer.onResize(windowWidth, windowHeight);
}

void Engine::startNextRound() {
    if (!m_simulation.roundActive()) {
        m_simulation.startNextRound();
    }
}

void Engine::selectTowerType(TowerType type) {
    m_hasPlacement = true;
    m_placementType = type;
}

void Engine::cancelPlacement() {
    m_hasPlacement = false;
}

void Engine::frame(int windowWidth, int windowHeight) {
    (void)windowWidth;
    (void)windowHeight;
    if (!m_running) {
        return;
    }

    m_clock.tick();

    PointerState ptr = m_input.pointerState();
    m_testScreen.setCursorPosition(ptr.logicalX, ptr.logicalY);

    if (m_input.isActionJustPressed(InputAction::StartRound)) {
        startNextRound();
    }

    if (m_input.isActionJustPressed(InputAction::Confirm)) {
        if (ptr.logicalX >= 404.0f && ptr.logicalX <= 476.0f) {
            float relY = ptr.logicalY - 22.0f;
            int idx = static_cast<int>(relY / 40.0f);
            if (idx >= 0 && idx < 6) {
                static const TowerType tts[] = {
                    TowerType::DartMonkey,
                    TowerType::TackShooter,
                    TowerType::BombTower,
                    TowerType::BoomerangThrower,
                    TowerType::SniperMonkey,
                    TowerType::SuperMonkey
                };
                selectTowerType(tts[idx]);
            }
        } else if (ptr.logicalX < 400.0f) {
            if (m_hasPlacement) {
                if (m_simulation.placeTower(m_placementType, ptr.logicalX, ptr.logicalY)) {
                    cancelPlacement();
                }
            } else {
                m_selectedTowerId = 0;
                for (const auto& tower : m_simulation.towers()) {
                    float dx = tower.x() - ptr.logicalX;
                    float dy = tower.y() - ptr.logicalY;
                    if (dx * dx + dy * dy <= 16.0f * 16.0f) {
                        m_selectedTowerId = tower.id();
                        break;
                    }
                }
            }
        }
    }

    if (m_input.isActionJustPressed(InputAction::Cancel)) {
        cancelPlacement();
        m_selectedTowerId = 0;
    }

    if (m_input.isActionJustPressed(InputAction::Pause)) {
        if (m_simulation.state() == GameStateType::Paused) {
            m_simulation.resume();
        } else if (m_simulation.state() == GameStateType::Playing) {
            m_simulation.pause();
        }
    }

    while (m_clock.hasFixedTick()) {
        if (m_gameMode) {
            m_simulation.update(static_cast<float>(m_clock.fixedTimeStep()));
        } else {
            m_testScreen.update(m_clock.fixedTimeStep());
        }
        m_clock.consumeFixedTick();
    }

    m_renderer.beginFrame();
    m_renderer.clear(Color::black());
    m_renderer.setViewport(m_viewport);

    if (m_gameMode) {
        AssetManager::instance().drawMap(m_renderer, m_simulation.map());

        for (const auto& tower : m_simulation.towers()) {
            AssetManager::instance().drawTower(m_renderer, tower, tower.id() == m_selectedTowerId);
        }

        if (m_hasPlacement && ptr.logicalX < 400.0f) {
            auto stats = getTowerBaseStats(m_placementType);
            bool canPlace = m_simulation.map().canPlaceTower(ptr.logicalX, ptr.logicalY, stats.footprintRadius) &&
                            m_simulation.economy().canAfford(stats.cost);
            Color rangeColor = canPlace ? Color::cyan() : Color::red();
            DebugRenderer::drawTowerRange(m_renderer, ptr.logicalX, ptr.logicalY, stats.range, rangeColor, {rangeColor.r, rangeColor.g, rangeColor.b, 30});
            Tower previewTower(0, m_placementType, ptr.logicalX, ptr.logicalY);
            AssetManager::instance().drawTower(m_renderer, previewTower, false);
        }

        std::vector<const Bloon*> activeBloons;
        m_simulation.bloonPool().getActiveBloons(activeBloons);
        for (const auto* bloon : activeBloons) {
            if (bloon) {
                AssetManager::instance().drawBloon(m_renderer, *bloon);
            }
        }

        for (const auto& proj : m_simulation.projectilePool().allProjectiles()) {
            if (proj.active) {
                AssetManager::instance().drawProjectile(m_renderer, proj);
            }
        }

        AssetManager::instance().drawHUD(m_renderer, m_simulation.economy(),
                                         m_simulation.currentRound(),
                                         m_simulation.completedRounds() + (m_simulation.roundActive() ? 1 : 0),
                                         m_clock.fps(), m_placementType, m_hasPlacement);

        if (!m_simulation.roundActive() && m_simulation.state() == GameStateType::Playing) {
            m_renderer.drawRect(118.0f, 228.0f, 160.0f, 30.0f, {0, 0, 0, 185}, true);
            m_renderer.drawRect(118.0f, 228.0f, 160.0f, 30.0f, Color::cyan(), false);
            m_renderer.drawText("R: START ROUND", 132.0f, 238.0f, 1.0f, Color::white());
        }

        if (m_simulation.state() == GameStateType::Paused) {
            m_renderer.drawRect(90.0f, 100.0f, 220.0f, 72.0f, {0, 0, 0, 210}, true);
            m_renderer.drawText("PAUSED", 170.0f, 118.0f, 2.0f, Color::white());
            m_renderer.drawText("Press P to resume", 135.0f, 145.0f, 1.0f, Color::cyan());
        } else if (m_simulation.state() == GameStateType::GameOver) {
            m_renderer.drawRect(70.0f, 92.0f, 260.0f, 88.0f, {0, 0, 0, 220}, true);
            m_renderer.drawText("GAME OVER", 145.0f, 112.0f, 2.0f, Color::red());
            m_renderer.drawText("Press ESC to exit", 135.0f, 145.0f, 1.0f, Color::white());
        } else if (m_simulation.state() == GameStateType::Victory) {
            m_renderer.drawRect(70.0f, 92.0f, 260.0f, 88.0f, {0, 0, 0, 220}, true);
            m_renderer.drawText("VICTORY!", 145.0f, 112.0f, 2.0f, Color::green());
            m_renderer.drawText("All rounds complete", 125.0f, 145.0f, 1.0f, Color::white());
        }
    } else {
        m_testScreen.render(m_renderer, m_clock.fps());
    }

    m_renderer.endFrame();
}

bool Engine::isRunning() const {
    return m_running;
}

void Engine::requestExit() {
    m_running = false;
}

Clock& Engine::clock() {
    return m_clock;
}

const Clock& Engine::clock() const {
    return m_clock;
}

const Viewport& Engine::currentViewport() const {
    return m_viewport;
}

} // namespace btd4
