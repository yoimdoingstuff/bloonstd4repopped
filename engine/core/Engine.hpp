#pragma once

#include "Clock.hpp"
#include "Logger.hpp"
#include "../rendering/IRenderer.hpp"
#include "../rendering/LogicalResolution.hpp"
#include "../rendering/TestScreen.hpp"
#include "../input/IInput.hpp"
#include "../input/InputActions.hpp"
#include "../input/FrontendProfile.hpp"
#include "../game/GameState.hpp"
#include "../game/Upgrade.hpp"
#include "../assets/AssetManager.hpp"

namespace btd4 {

class Engine {
public:
    Engine(IRenderer& renderer, IInput& input,
           FrontendProfile frontendProfile = FrontendProfile::FlashDesktop);
    ~Engine();

    bool initialize(int windowWidth, int windowHeight);
    void shutdown();
    void frame(int windowWidth, int windowHeight);
    void onResize(int windowWidth, int windowHeight);

    bool isRunning() const;
    void requestExit();

    Clock& clock();
    const Clock& clock() const;
    const Viewport& currentViewport() const;
    FrontendProfile frontendProfile() const { return m_frontendProfile; }

    GameSimulation& simulation() { return m_simulation; }
    const GameSimulation& simulation() const { return m_simulation; }

    void startNextRound();
    void selectTowerType(TowerType type);
    void cancelPlacement();

private:
    bool applySelectedUpgrade(uint8_t path);

    IRenderer& m_renderer;
    IInput& m_input;
    FrontendProfile m_frontendProfile{FrontendProfile::FlashDesktop};
    Clock m_clock;
    TestScreen m_testScreen;
    GameSimulation m_simulation;
    UpgradeSet m_upgrades;

    Viewport m_viewport;
    bool m_running{false};
    bool m_gameMode{true};
    bool m_hasPlacement{false};
    TowerType m_placementType{TowerType::DartMonkey};
    uint32_t m_selectedTowerId{0};
};

} // namespace btd4
