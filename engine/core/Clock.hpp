#pragma once

#include <chrono>
#include <cstdint>

namespace btd4 {

class Clock {
public:
    Clock(double fixedTimeStep = 1.0 / 60.0);

    void reset();
    void tick();

    // Time deltas
    double deltaTime() const;
    double fixedTimeStep() const;
    void setFixedTimeStep(double step);

    // Simulation tick accumulation
    bool hasFixedTick();
    void consumeFixedTick();

    // Frame & rate metrics
    double fps() const;
    uint64_t frameCount() const;
    double totalTime() const;

private:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    TimePoint m_startTime;
    TimePoint m_lastTime;

    double m_deltaTime{0.0};
    double m_fixedTimeStep{1.0 / 60.0};
    double m_accumulator{0.0};

    uint64_t m_frameCount{0};
    double m_fps{0.0};
    double m_fpsTimer{0.0};
    uint32_t m_fpsFrameCounter{0};
    double m_totalTime{0.0};
};

} // namespace btd4
