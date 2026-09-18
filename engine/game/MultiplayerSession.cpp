#include "MultiplayerSession.hpp"

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

Player& MultiplayerSession::player(uint8_t id) {
    return m_players.at(id);
}

const Player& MultiplayerSession::player(uint8_t id) const {
    return m_players.at(id);
}

} // namespace btd4
