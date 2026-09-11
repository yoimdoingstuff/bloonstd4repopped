#pragma once

#include "Clock.hpp"
#include "Logger.hpp"
#include "../rendering/IRenderer.hpp"
#include "../rendering/LogicalResolution.hpp"
#include "../rendering/TestScreen.hpp"
#include "../input/IInput.hpp"
#include "../input/InputActions.hpp"
#include "../game/GameState.hpp"
#include "../assets/AssetManager.hpp"

namespace btd4 {

class Engine {
public:
    Engine(IRenderer& renderer, IInput& input);
    ~Engine();

    bool initialize(int windowWidth, int windowHeight);
    void shutdown();

    // Executes a single simulation & render frame
    void frame(int windowWidth, int windowHeight);

    void onResize(int windowWidth, int windowHeight);

    bool isRunning() const;
    void requestExit();

    Clock& clock();
    const Clock& clock() const;

    const Viewport& currentViewport() const;

    GameSimulation& simulation() { return m_simulation; }
    const GameSimulation& simulation() const { return m_simulation; }

    void startNextRound();
    void selectTowerType(TowerType type);
    void cancelPlacement();

private:
    IRenderer& m_renderer;
    IInput& m_input;
    Clock m_clock;
    TestScreen m_testScreen;
    GameSimulation m_simulation;

    Viewport m_viewport;
    bool m_running{false};
    bool m_gameMode{true};

    bool m_hasPlacement{false};
    TowerType m_placementType{TowerType::DartMonkey};
    uint32_t m_selectedTowerId{0};
};

} // namespace btd4
