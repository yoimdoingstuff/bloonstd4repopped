#include "Rounds.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace btd4 {
bool validateRounds(const RoundSet& rounds, const Map& map, std::string& error) {
    error.clear();
    if (!map.validate()) { error = "Rounds require a valid map"; return false; }
    if (rounds.rounds.empty() || rounds.rounds.size() > 1024) {
        error = "Expected 1..1024 rounds"; return false;
    }
    for (const auto& round : rounds.rounds) {
        if (round.groups.empty() || round.groups.size() > 1024) {
            error = "Expected 1..1024 groups per round"; return false;
        }
        uint64_t duration = 0, count = 0;
        for (const auto& group : round.groups) {
            if (group.type < BloonType::Red || group.type > BloonType::MOAB ||
                group.count == 0 || group.count > 100000 ||
                group.spacingMs > 3600000 || group.delayMs > 3600000 ||
                group.pathIndex >= map.paths().size()) {
                error = "Invalid bloon group type, count, timing or path"; return false;
            }
            duration += group.delayMs + uint64_t(group.count - 1) * group.spacingMs;
            count += group.count;
            if (duration > 3600000 || count > 100000) {
                error = "Round exceeds one hour or 100000 scheduled bloons"; return false;
            }
        }
    }
    return true;
}
bool RoundScheduler::configure(RoundSet rounds, const Map& map, std::string& error) {
    if (m_active) { error = "Cannot replace active rounds"; return false; }
    if (!validateRounds(rounds, map, error)) return false;
    m_rounds = std::move(rounds);
    reset();
    return true;
}
void RoundScheduler::reset() {
    m_nextRound = m_group = 0;
    m_spawned = 0;
    m_dueMs = 0;
    m_elapsedMs = 0;
    m_active = false;
}
bool RoundScheduler::start(BloonPool& pool, const Map& map) {
    if (m_active || pool.activeCount() != 0 || m_nextRound >= m_rounds.rounds.size()) return false;
    // Recheck paths in case the caller changed the map between rounds.
    std::string error;
    if (!validateRounds(m_rounds, map, error)) return false;
    m_group = 0;
    m_spawned = 0;
    m_elapsedMs = 0;
    m_dueMs = m_rounds.rounds[m_nextRound].groups.front().delayMs;
    m_active = true;
    advance(0, pool, map);
    return true;
}
bool RoundScheduler::advance(double seconds, BloonPool& pool, const Map& map) {
    if (!m_active || !std::isfinite(seconds) || seconds < 0) return false;
    m_elapsedMs = std::min(3600000.0, m_elapsedMs + seconds * 1000.0);
    const auto& groups = m_rounds.rounds[m_nextRound].groups;
    while (m_group < groups.size() && double(m_dueMs) <= m_elapsedMs + 1e-7) {
        const auto& group = groups[m_group];
        if (group.pathIndex >= map.paths().size()) return false;
        Bloon* bloon = pool.spawn(group.type, group.pathIndex);
        if (!bloon) return false; // Retry this exact spawn when capacity frees up.
        const auto position = map.paths()[group.pathIndex].getPositionAtDistance(0);
        bloon->x = position.x;
        bloon->y = position.y;
        if (++m_spawned == group.count) {
            m_spawned = 0;
            if (++m_group < groups.size()) m_dueMs += groups[m_group].delayMs;
        } else m_dueMs += group.spacingMs;
    }
    if (m_group == groups.size() && pool.activeCount() == 0) {
        m_active = false;
        ++m_nextRound;
        return true;
    }
    return false;
}
}
