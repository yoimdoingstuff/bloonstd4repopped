#pragma once

#include <vector>
#include <memory>
#include "Bloon.hpp"
#include "Tower.hpp"
#include "Projectile.hpp"
#include "Economy.hpp"
#include "../map/Map.hpp"

namespace btd4 {

enum class GameStateType {
    MainMenu,
    Playing,
    Paused,
    GameOver,
    Victory
};

class GameSimulation {
public:
    GameSimulation();
    explicit GameSimulation(Map map);

    void reset();

    GameStateType state() const { return m_state; }
    void setState(GameStateType s) { m_state = s; }

    void pause();
    void resume();

    const Map& map() const { return m_map; }
    Map& map() { return m_map; }
    void setMap(Map map) { m_map = std::move(map); }

    BloonPool& bloonPool() { return m_bloonPool; }
    const BloonPool& bloonPool() const { return m_bloonPool; }

    ProjectilePool& projectilePool() { return m_projectilePool; }
    const ProjectilePool& projectilePool() const { return m_projectilePool; }

    Economy& economy() { return m_economy; }
    const Economy& economy() const { return m_economy; }

    const std::vector<Tower>& towers() const { return m_towers; }
    std::vector<Tower>& towers() { return m_towers; }

    // Tower management
    bool placeTower(TowerType type, float x, float y);
    bool sellTower(uint32_t towerId);
    Tower* findTower(uint32_t towerId);

    // Simulation tick
    void update(float deltaTime);

    int totalBloonsPopped() const { return m_totalBloonsPopped; }
    int totalBloonsLeaked() const { return m_totalBloonsLeaked; }
    int currentRound() const { return m_currentRound; }
    void setCurrentRound(int round) { m_currentRound = round; }

private:
    GameStateType m_state{GameStateType::MainMenu};
    Map m_map;
    BloonPool m_bloonPool;
    ProjectilePool m_projectilePool;
    std::vector<Tower> m_towers;
    Economy m_economy;

    uint32_t m_nextTowerId{1};
    int m_currentRound{1};
    int m_totalBloonsPopped{0};
    int m_totalBloonsLeaked{0};
};

} // namespace btd4
