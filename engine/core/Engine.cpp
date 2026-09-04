#include "Engine.hpp"

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

void Engine::frame(int windowWidth, int windowHeight) {
    if (!m_running) {
        return;
    }

    // Tick the clock
    m_clock.tick();

    if (m_input.isActionJustPressed(InputAction::Pause)) {
        BTD4_LOG_INFO("Pause action triggered.");
    }

    // Pass pointer coordinate to test screen
    PointerState ptr = m_input.pointerState();
    m_testScreen.setCursorPosition(ptr.logicalX, ptr.logicalY);

    // Fixed timestep simulation updates
    while (m_clock.hasFixedTick()) {
        m_testScreen.update(m_clock.fixedTimeStep());
        m_clock.consumeFixedTick();
    }

    // Render frame
    m_renderer.beginFrame();

    // Outer letterbox clear (bars)
    m_renderer.clear(Color::black());

    // Set 480x272 viewport area
    m_renderer.setViewport(m_viewport);

    // Render test screen content
    m_testScreen.render(m_renderer, m_clock.fps());

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
