#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Bloon.hpp"
#include "Projectile.hpp"

namespace btd4 {

enum class TowerType : uint8_t {
    DartMonkey = 0,
    TackShooter,
    SniperMonkey,
    BoomerangThrower,
    BombTower,
    SuperMonkey
};

enum class TargetingMode : uint8_t {
    First = 0,
    Last,
    Close,
    Strong
};

struct TowerBaseStats {
    int cost{200};
    float range{100.0f};
    float attackCooldown{0.95f}; // Seconds between attacks
    float footprintRadius{12.0f};
    ProjectileType projectileType{ProjectileType::Dart};
    DamageType damageType{DamageType::Sharp};
    int projectileDamage{1};
    int projectilePierce{1};
    float projectileSpeed{240.0f};
    float explosionRadius{0.0f};
};

TowerBaseStats getTowerBaseStats(TowerType type);

class Tower {
public:
    Tower(uint32_t id, TowerType type, float x, float y);

    uint32_t id() const { return m_id; }
    TowerType type() const { return m_type; }

    float x() const { return m_x; }
    float y() const { return m_y; }
    void setPosition(float x, float y) { m_x = x; m_y = y; }

    float range() const { return m_range; }
    void setRange(float r) { m_range = r; }

    float attackCooldown() const { return m_attackCooldown; }
    void setAttackCooldown(float cd) { m_attackCooldown = cd; }

    TargetingMode targetingMode() const { return m_targetingMode; }
    void setTargetingMode(TargetingMode mode) { m_targetingMode = mode; }
    void cycleTargetingMode();

    int totalInvestedCost() const { return m_totalInvestedCost; }
    void addInvestedCost(int cost) { m_totalInvestedCost += cost; }
    int sellValue() const { return static_cast<int>(m_totalInvestedCost * 0.75f); }

    bool canAttack() const { return m_cooldownTimer <= 0.0f; }
    void resetCooldown() { m_cooldownTimer = m_attackCooldown; }
    void updateCooldown(float deltaTime);

    // Filters and chooses the best target from available active bloons
    const Bloon* selectTarget(const std::vector<Bloon*>& activeBloons) const;

    // Executes an attack if off cooldown and valid target exists
    bool attack(BloonPool& bloonPool, ProjectilePool& projectilePool, const Map& map);

private:
    uint32_t m_id{0};
    TowerType m_type{TowerType::DartMonkey};
    float m_x{0.0f};
    float m_y{0.0f};
    float m_range{100.0f};
    float m_attackCooldown{1.0f};
    float m_cooldownTimer{0.0f};
    TargetingMode m_targetingMode{TargetingMode::First};
    int m_totalInvestedCost{200};
    TowerBaseStats m_stats;
};

} // namespace btd4
