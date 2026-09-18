#pragma once

#include "Player.hpp"
#include <array>
#include <cstdint>
#include <string>

namespace btd4 {

enum class MultiplayerEconomyMode : uint8_t {
    Shared = 0,
    Split
};

class MultiplayerSession {
public:
    bool configure(uint8_t playerCount, MultiplayerEconomyMode economyMode, std::string& error);

    bool start(std::string& error);
    void stop();

    bool assignController(uint8_t playerId, int32_t controllerInstanceId, std::string& error);
    void unassignController(int32_t controllerInstanceId);
    int playerForController(int32_t controllerInstanceId) const;

    bool active() const { return m_active; }
    uint8_t playerCount() const { return m_playerCount; }
    MultiplayerEconomyMode economyMode() const { return m_economyMode; }

    Player& player(uint8_t id);
    const Player& player(uint8_t id) const;

private:
    std::array<Player, Player::MaxPlayers> m_players{};
    uint8_t m_playerCount{1};
    MultiplayerEconomyMode m_economyMode{MultiplayerEconomyMode::Split};
    bool m_active{false};
};

} // namespace btd4
