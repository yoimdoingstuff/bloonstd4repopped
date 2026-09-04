#include "GameState.hpp"
#include <algorithm>

namespace btd4 {

GameSimulation::GameSimulation()
    : m_state(GameStateType::MainMenu) {
}

GameSimulation::GameSimulation(Map map)
    : m_state(GameStateType::Playing), m_map(std::move(map)) {
}

void GameSimulation::reset() {
    m_bloonPool.clear();
    m_projectilePool.clear();
    m_towers.clear();
    m_economy.reset();
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

void GameSimulation::update(float deltaTime) {
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
}

} // namespace btd4
