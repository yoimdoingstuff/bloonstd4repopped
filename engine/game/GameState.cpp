#include "GameState.hpp"
#include <algorithm>
#include <cmath>

namespace btd4 {

GameSimulation::GameSimulation()
    : m_state(GameStateType::MainMenu) {
    for (size_t i = 0; i < m_players.size(); ++i)
        m_players[i] = Player(static_cast<uint8_t>(i), "Player " + std::to_string(i + 1));
    m_players[0].setActive(true);
    for (size_t i = 1; i < m_players.size(); ++i) m_players[i].setActive(false);
}

GameSimulation::GameSimulation(Map map)
    : m_state(GameStateType::Playing), m_map(std::move(map)) {
    for (size_t i = 0; i < m_players.size(); ++i)
        m_players[i] = Player(static_cast<uint8_t>(i), "Player " + std::to_string(i + 1));
    for (size_t i = 1; i < m_players.size(); ++i) m_players[i].setActive(false);
}

void GameSimulation::reset() {
    m_bloonPool.clear();
    m_projectilePool.clear();
    m_towers.clear();
    for (size_t i = 0; i < m_players.size(); ++i) {
        m_players[i].reset();
        m_players[i].setActive(i == 0);
    }
    m_rounds.reset();
    m_state = GameStateType::Playing;
    m_nextTowerId = 1;
    m_currentRound = 1;
    m_totalBloonsPopped = 0;
    m_totalBloonsLeaked = 0;
}

void GameSimulation::pause() {
    if (m_state == GameStateType::Playing) {
        m_state = GameStateType::Paused;
    }
}

void GameSimulation::resume() {
    if (m_state == GameStateType::Paused) {
        m_state = GameStateType::Playing;
    }
}

bool GameSimulation::placeTower(TowerType type, float x, float y) {
    TowerBaseStats stats = getTowerBaseStats(type);
    if (!m_economy.canAfford(stats.cost)) {
        return false;
    }

    if (!m_map.canPlaceTower(x, y, stats.footprintRadius)) {
        return false;
    }

    // Check collision with existing towers
    for (const auto& existing : m_towers) {
        float dx = existing.x() - x;
        float dy = existing.y() - y;
        float minDist = stats.footprintRadius + 12.0f; // Minimal spacing
        if ((dx * dx + dy * dy) < (minDist * minDist)) {
            return false;
        }
    }

    m_economy.spendCash(stats.cost);
    m_towers.emplace_back(m_nextTowerId++, type, x, y);
    return true;
}

bool GameSimulation::sellTower(uint32_t towerId) {
    for (auto it = m_towers.begin(); it != m_towers.end(); ++it) {
        if (it->id() == towerId) {
            int refund = it->sellValue();
            m_economy.addCash(refund);
            m_towers.erase(it);
            return true;
        }
    }
    return false;
}

Tower* GameSimulation::findTower(uint32_t towerId) {
    for (auto& t : m_towers) {
        if (t.id() == towerId) {
            return &t;
        }
    }
    return nullptr;
}

bool GameSimulation::setRounds(RoundSet rounds, std::string& error) {
    if (m_rounds.active() || m_rounds.completedRounds() != 0 ||
        m_bloonPool.activeCount() != 0 || m_projectilePool.activeCount() != 0) {
        error = "Reset the simulation before replacing rounds";
        return false;
    }
    if (!m_rounds.configure(std::move(rounds), m_map, error)) return false;
    m_currentRound = 1;
    return true;
}

bool GameSimulation::startNextRound() {
    if (m_state != GameStateType::Playing || m_economy.isDefeated()) return false;
    if (!m_rounds.start(m_bloonPool, m_map)) return false;
    m_currentRound = static_cast<int>(m_rounds.completedRounds() + 1);
    m_projectilePool.clear();
    return true;
}

void GameSimulation::update(float deltaTime) {
    if (!std::isfinite(deltaTime) || deltaTime <= 0) return;
    if (m_state != GameStateType::Playing) {
        return;
    }

    // 1. Update bloons along paths
    std::vector<uint32_t> leakedBloons;
    m_bloonPool.update(deltaTime, m_map, leakedBloons);

    for (uint32_t id : leakedBloons) {
        (void)id;
        // 1 life penalty per leaked red bloon or equivalent
        m_economy.loseLives(1);
        m_totalBloonsLeaked++;
    }

    if (m_economy.isDefeated()) {
        m_state = GameStateType::GameOver;
        return;
    }

    // 2. Update towers and fire attacks
    for (auto& tower : m_towers) {
        tower.updateCooldown(deltaTime);
        tower.attack(m_bloonPool, m_projectilePool, m_map);
    }

    // 3. Update projectiles and resolve collisions
    int cashEarned = m_projectilePool.update(deltaTime, m_bloonPool, m_map);
    if (cashEarned > 0) {
        m_economy.addCash(cashEarned);
        m_totalBloonsPopped += cashEarned;
    }

    // Spawn at the end of this tick: a new bloon must not move for time before
    // it existed. Deadlines are quantized to the caller's fixed tick boundary.
    if (m_rounds.advance(deltaTime, m_bloonPool, m_map)) {
        m_economy.addCash(Economy::calculateRoundReward(
            static_cast<int>(m_rounds.completedRounds())));
        m_projectilePool.clear();
        if (m_rounds.finished()) m_state = GameStateType::Victory;
    }
}

} // namespace btd4
