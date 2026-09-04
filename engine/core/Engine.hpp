#pragma once

#include "Clock.hpp"
#include "Logger.hpp"
#include "../rendering/IRenderer.hpp"
#include "../rendering/LogicalResolution.hpp"
#include "../rendering/TestScreen.hpp"
#include "../input/IInput.hpp"

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

private:
    IRenderer& m_renderer;
    IInput& m_input;
    Clock m_clock;
    TestScreen m_testScreen;

    Viewport m_viewport;
    bool m_running{false};
};

} // namespace btd4
