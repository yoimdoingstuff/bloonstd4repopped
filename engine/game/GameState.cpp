#include "GameState.hpp"
#include <algorithm>
#include <cmath>
#include <string>

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
    m_activePlayerId = 0;
    m_currentRound = 1;
    m_totalBloonsPopped = 0;
    m_totalBloonsLeaked = 0;
}

Economy& GameSimulation::economyForPlayer(uint8_t playerId) {
    return m_economyMode == MultiplayerEconomyMode::Shared
        ? m_players[0].economy()
        : m_players[playerId].economy();
}

const Economy& GameSimulation::economyForPlayer(uint8_t playerId) const {
    return m_economyMode == MultiplayerEconomyMode::Shared
        ? m_players[0].economy()
        : m_players[playerId].economy();
}

void GameSimulation::syncSharedEconomy() {
    if (m_economyMode != MultiplayerEconomyMode::Shared) return;
    const int cash = m_players[0].economy().cash();
    const int lives = m_players[0].economy().lives();
    for (size_t i = 1; i < m_players.size(); ++i) {
        if (m_players[i].active())
            m_players[i].economy().reset(cash, lives);
    }
}

bool GameSimulation::setActivePlayer(uint8_t playerId) {
    if (playerId >= MAX_PLAYERS || !m_players[playerId].active()) return false;
    m_activePlayerId = playerId;
    return true;
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
    return placeTower(m_activePlayerId, type, x, y);
}

bool GameSimulation::placeTower(uint8_t playerId, TowerType type, float x, float y) {
    if (playerId >= MAX_PLAYERS || !m_players[playerId].active()) return false;

    TowerBaseStats stats = getTowerBaseStats(type);
    Economy& playerEconomy = economyForPlayer(playerId);
    if (!playerEconomy.canAfford(stats.cost)) {
        return false;
    }

    if (!m_map.canPlaceTower(x, y, stats.footprintRadius)) {
        return false;
    }

    // Check collision with existing towers
    for (const auto& existing : m_towers) {
        float dx = existing.x() - x;
        float dy = existing.y() - y;
        float minDist = stats.footprintRadius + 12.0f;
        if ((dx * dx + dy * dy) < (minDist * minDist)) {
            return false;
        }
    }

    playerEconomy.spendCash(stats.cost);
    syncSharedEconomy();
    m_towers.emplace_back(m_nextTowerId++, type, x, y, playerId);
    return true;
}

bool GameSimulation::sellTower(uint32_t towerId) {
    return sellTower(m_activePlayerId, towerId);
}

bool GameSimulation::sellTower(uint8_t playerId, uint32_t towerId) {
    if (playerId >= MAX_PLAYERS || !m_players[playerId].active()) return false;
    for (auto it = m_towers.begin(); it != m_towers.end(); ++it) {
        if (it->id() == towerId && it->ownerId() == playerId) {
            int refund = it->sellValue();
            economyForPlayer(playerId).addCash(refund);
            syncSharedEconomy();
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
    if (m_state != GameStateType::Playing || economy().isDefeated()) return false;
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
        economyForPlayer(m_activePlayerId).loseLives(1);
        syncSharedEconomy();
        m_totalBloonsLeaked++;
    }

    if (economy().isDefeated()) {
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
        economyForPlayer(m_activePlayerId).addCash(cashEarned);
        syncSharedEconomy();
        m_totalBloonsPopped += cashEarned;
    }

    // Spawn at the end of this tick: a new bloon must not move for time before
    // it existed. Deadlines are quantized to the caller's fixed tick boundary.
    if (m_rounds.advance(deltaTime, m_bloonPool, m_map)) {
        economyForPlayer(m_activePlayerId).addCash(Economy::calculateRoundReward(
            static_cast<int>(m_rounds.completedRounds())));
        syncSharedEconomy();
        m_projectilePool.clear();
        if (m_rounds.finished()) m_state = GameStateType::Victory;
    }
}

} // namespace btd4
