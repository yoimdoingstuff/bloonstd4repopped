#include "Clock.hpp"
#include <algorithm>

namespace btd4 {

Clock::Clock(double fixedTimeStep)
    : m_fixedTimeStep(fixedTimeStep) {
    reset();
}

void Clock::reset() {
    m_startTime = std::chrono::high_resolution_clock::now();
    m_lastTime = m_startTime;
    m_deltaTime = 0.0;
    m_accumulator = 0.0;
    m_frameCount = 0;
    m_fps = 0.0;
    m_fpsTimer = 0.0;
    m_fpsFrameCounter = 0;
    m_totalTime = 0.0;
}

void Clock::tick() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - m_lastTime;
    m_lastTime = now;

    m_deltaTime = elapsed.count();
    // Clamp delta time to avoid spiral of death if paused/stalled
    if (m_deltaTime > 0.25) {
        m_deltaTime = 0.25;
    }

    m_totalTime += m_deltaTime;
    m_accumulator += m_deltaTime;
    m_frameCount++;

    m_fpsTimer += m_deltaTime;
    m_fpsFrameCounter++;
    if (m_fpsTimer >= 0.5) {
        m_fps = static_cast<double>(m_fpsFrameCounter) / m_fpsTimer;
        m_fpsFrameCounter = 0;
        m_fpsTimer = 0.0;
    }
}

double Clock::deltaTime() const {
    return m_deltaTime;
}

double Clock::fixedTimeStep() const {
    return m_fixedTimeStep;
}

void Clock::setFixedTimeStep(double step) {
    if (step > 0.0) {
        m_fixedTimeStep = step;
    }
}

bool Clock::hasFixedTick() {
    return m_accumulator >= m_fixedTimeStep;
}

void Clock::consumeFixedTick() {
    if (m_accumulator >= m_fixedTimeStep) {
        m_accumulator -= m_fixedTimeStep;
    }
}

double Clock::fps() const {
    return m_fps;
}

uint64_t Clock::frameCount() const {
    return m_frameCount;
}

double Clock::totalTime() const {
    return m_totalTime;
}

} // namespace btd4
