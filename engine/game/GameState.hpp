#pragma once

#include <vector>
#include <memory>
#include "Bloon.hpp"
#include "Tower.hpp"
#include "Projectile.hpp"
#include "Economy.hpp"
#include "Player.hpp"
#include "MultiplayerSession.hpp"
#include <array>
#include "Rounds.hpp"
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

    Economy& economy() { return m_players[0].economy(); }
    const Economy& economy() const { return m_players[0].economy(); }

    static constexpr size_t MAX_PLAYERS = Player::MaxPlayers;
    Player& player(size_t index) { return m_players.at(index); }
    const Player& player(size_t index) const { return m_players.at(index); }
    std::array<Player, MAX_PLAYERS>& players() { return m_players; }
    const std::array<Player, MAX_PLAYERS>& players() const { return m_players; }

    void setEconomyMode(MultiplayerEconomyMode mode) { m_economyMode = mode; syncSharedEconomy(); }
    MultiplayerEconomyMode economyMode() const { return m_economyMode; }

    const std::vector<Tower>& towers() const { return m_towers; }
    std::vector<Tower>& towers() { return m_towers; }

    // Tower management
    bool placeTower(TowerType type, float x, float y);
    bool placeTower(uint8_t playerId, TowerType type, float x, float y);
    bool sellTower(uint32_t towerId);
    bool sellTower(uint8_t playerId, uint32_t towerId);
    Tower* findTower(uint32_t towerId);

    // Configure before the first round (or after reset). Manual spawning remains
    // available when no round set is configured. Do not edit the map mid-round.
    bool setRounds(RoundSet rounds, std::string& error);
    bool startNextRound();
    bool roundActive() const { return m_rounds.active(); }
    size_t completedRounds() const { return m_rounds.completedRounds(); }

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
    std::array<Player, MAX_PLAYERS> m_players{};
    uint8_t m_activePlayerId{0};
    MultiplayerEconomyMode m_economyMode{MultiplayerEconomyMode::Split};

    Economy& economyForPlayer(uint8_t playerId);
    const Economy& economyForPlayer(uint8_t playerId) const;
    void syncSharedEconomy();
    RoundScheduler m_rounds;

    uint32_t m_nextTowerId{1};
    int m_currentRound{1};
    int m_totalBloonsPopped{0};
    int m_totalBloonsLeaked{0};
};

} // namespace btd4
