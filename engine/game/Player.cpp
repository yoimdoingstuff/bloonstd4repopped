#include "Player.hpp"
#include <utility>

namespace btd4 {

Player::Player(uint8_t id, std::string name)
    : m_id(id), m_name(std::move(name)) {
    if (m_id >= MaxPlayers) m_id = 0;
    if (m_name.empty()) m_name = "Player " + std::to_string(static_cast<int>(m_id) + 1);
}

void Player::setName(std::string name) {
    if (!name.empty()) m_name = std::move(name);
}

void Player::reset(int cash, int lives) {
    m_active = true;
    m_controllerInstanceId = -1;
    m_economy.reset(cash, lives);
}

} // namespace btd4
