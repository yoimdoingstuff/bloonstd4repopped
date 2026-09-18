#include "MultiplayerSession.hpp"
#include <string>

namespace btd4 {

bool MultiplayerSession::configure(uint8_t playerCount,
                                   MultiplayerEconomyMode economyMode,
                                   std::string& error) {
    error.clear();
    if (m_active) {
        error = "Stop the multiplayer session before reconfiguring it";
        return false;
    }
    if (playerCount < 1 || playerCount > Player::MaxPlayers) {
        error = "Player count must be between 1 and 4";
        return false;
    }

    m_playerCount = playerCount;
    m_economyMode = economyMode;
    for (uint8_t i = 0; i < Player::MaxPlayers; ++i) {
        m_players[i] = Player(i, "Player " + std::to_string(static_cast<int>(i) + 1));
        m_players[i].setActive(i < m_playerCount);
    }
    return true;
}

bool MultiplayerSession::start(std::string& error) {
    error.clear();
    if (m_active) return true;
    if (m_playerCount < 1 || m_playerCount > Player::MaxPlayers) {
        error = "Invalid multiplayer player count";
        return false;
    }
    for (uint8_t i = 0; i < Player::MaxPlayers; ++i)
        m_players[i].setActive(i < m_playerCount);
    m_active = true;
    return true;
}

void MultiplayerSession::stop() {
    m_active = false;
    for (uint8_t i = m_playerCount; i < Player::MaxPlayers; ++i)
        m_players[i].setActive(false);
}

bool MultiplayerSession::assignController(uint8_t playerId,
                                             int32_t controllerInstanceId,
                                             std::string& error) {
    error.clear();
    if (playerId >= m_playerCount || !m_players[playerId].active()) {
        error = "Player is not active in this session";
        return false;
    }
    if (controllerInstanceId < 0) {
        error = "Controller instance ID must be non-negative";
        return false;
    }
    const int existing = playerForController(controllerInstanceId);
    if (existing >= 0 && existing != static_cast<int>(playerId)) {
        error = "Controller is already assigned to another player";
        return false;
    }
    for (uint8_t i = 0; i < m_playerCount; ++i) {
        if (i != playerId && m_players[i].controllerInstanceId() == controllerInstanceId) {
            error = "Controller is already assigned to another player";
            return false;
        }
    }
    m_players[playerId].setControllerInstanceId(controllerInstanceId);
    return true;
}

void MultiplayerSession::unassignController(int32_t controllerInstanceId) {
    for (auto& player : m_players) {
        if (player.controllerInstanceId() == controllerInstanceId)
            player.setControllerInstanceId(-1);
    }
}

int MultiplayerSession::playerForController(int32_t controllerInstanceId) const {
    for (uint8_t i = 0; i < m_playerCount; ++i) {
        if (m_players[i].controllerInstanceId() == controllerInstanceId)
            return static_cast<int>(i);
    }
    return -1;
}

Player& MultiplayerSession::player(uint8_t id) {
    return m_players.at(id);
}

const Player& MultiplayerSession::player(uint8_t id) const {
    return m_players.at(id);
}

} // namespace btd4
