#pragma once

#include <cstdint>

namespace btd4 {

class Economy {
public:
    Economy(int initialCash = 650, int initialLives = 100);

    void reset(int cash = 650, int lives = 100);

    int cash() const { return m_cash; }
    int lives() const { return m_lives; }

    bool canAfford(int cost) const { return m_cash >= cost; }
    bool spendCash(int cost);
    void addCash(int amount);

    void loseLives(int count);
    void addLives(int count, int maxLives = 1000);

    bool isDefeated() const { return m_lives <= 0; }

    // Computes cash bonus awarded upon round completion (BTD4 standard formula: 100 + round)
    static int calculateRoundReward(int roundNumber);

private:
    int m_cash{650};
    int m_lives{100};
};

} // namespace btd4
