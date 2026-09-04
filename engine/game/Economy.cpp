#include "Economy.hpp"
#include <algorithm>

namespace btd4 {

Economy::Economy(int initialCash, int initialLives)
    : m_cash(initialCash), m_lives(initialLives) {
}

void Economy::reset(int cash, int lives) {
    m_cash = cash;
    m_lives = lives;
}

bool Economy::spendCash(int cost) {
    if (cost < 0 || m_cash < cost) {
        return false;
    }
    m_cash -= cost;
    return true;
}

void Economy::addCash(int amount) {
    if (amount > 0) {
        m_cash += amount;
    }
}

void Economy::loseLives(int count) {
    if (count > 0) {
        m_lives = std::max(0, m_lives - count);
    }
}

void Economy::addLives(int count, int maxLives) {
    if (count > 0 && m_lives > 0) {
        m_lives = std::min(maxLives, m_lives + count);
    }
}

int Economy::calculateRoundReward(int roundNumber) {
    if (roundNumber <= 0) return 100;
    return 100 + roundNumber;
}

} // namespace btd4
