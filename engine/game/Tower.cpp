#include "Tower.hpp"
#include <cmath>
#include <algorithm>

namespace btd4 {

TowerBaseStats getTowerBaseStats(TowerType type) {
    TowerBaseStats stats;
    switch (type) {
        case TowerType::DartMonkey:
            stats.cost = 200;
            stats.range = 100.0f;
            stats.attackCooldown = 0.95f;
            stats.projectileType = ProjectileType::Dart;
            stats.damageType = DamageType::Sharp;
            stats.projectileDamage = 1;
            stats.projectilePierce = 1;
            stats.projectileSpeed = 260.0f;
            break;
        case TowerType::TackShooter:
            stats.cost = 250;
            stats.range = 70.0f;
            stats.attackCooldown = 1.25f;
            stats.projectileType = ProjectileType::Tack;
            stats.damageType = DamageType::Sharp;
            stats.projectileDamage = 1;
            stats.projectilePierce = 1;
            stats.projectileSpeed = 220.0f;
            break;
        case TowerType::SniperMonkey:
            stats.cost = 350;
            stats.range = 9999.0f; // Infinite map range
            stats.attackCooldown = 1.6f;
            stats.projectileType = ProjectileType::SniperShot;
            stats.damageType = DamageType::Sharp;
            stats.projectileDamage = 2;
            stats.projectilePierce = 1;
            stats.projectileSpeed = 2000.0f;
            break;
        case TowerType::BoomerangThrower:
            stats.cost = 350;
            stats.range = 110.0f;
            stats.attackCooldown = 1.1f;
            stats.projectileType = ProjectileType::Boomerang;
            stats.damageType = DamageType::Sharp;
            stats.projectileDamage = 1;
            stats.projectilePierce = 3;
            stats.projectileSpeed = 200.0f;
            break;
        case TowerType::BombTower:
            stats.cost = 550;
            stats.range = 110.0f;
            stats.attackCooldown = 1.35f;
            stats.projectileType = ProjectileType::Bomb;
            stats.damageType = DamageType::Explosive;
            stats.projectileDamage = 1;
            stats.projectilePierce = 1;
            stats.projectileSpeed = 200.0f;
            stats.explosionRadius = 35.0f;
            break;
        case TowerType::SuperMonkey:
            stats.cost = 3000;
            stats.range = 135.0f;
            stats.attackCooldown = 0.055f; // Rapid fire!
            stats.projectileType = ProjectileType::Dart;
            stats.damageType = DamageType::Sharp;
            stats.projectileDamage = 1;
            stats.projectilePierce = 1;
            stats.projectileSpeed = 350.0f;
            break;
        default:
            break;
    }
    return stats;
}

Tower::Tower(uint32_t id, TowerType type, float x, float y)
    : m_id(id), m_type(type), m_x(x), m_y(y) {
    m_stats = getTowerBaseStats(type);
    m_range = m_stats.range;
    m_attackCooldown = m_stats.attackCooldown;
    m_cooldownTimer = 0.0f;
    m_totalInvestedCost = m_stats.cost;
}

void Tower::cycleTargetingMode() {
    switch (m_targetingMode) {
        case TargetingMode::First:  m_targetingMode = TargetingMode::Last; break;
        case TargetingMode::Last:   m_targetingMode = TargetingMode::Close; break;
        case TargetingMode::Close:  m_targetingMode = TargetingMode::Strong; break;
        case TargetingMode::Strong: m_targetingMode = TargetingMode::First; break;
    }
}

void Tower::updateCooldown(float deltaTime) {
    if (m_cooldownTimer > 0.0f) {
        m_cooldownTimer -= deltaTime;
    }
}

const Bloon* Tower::selectTarget(const std::vector<Bloon*>& activeBloons) const {
    const Bloon* bestTarget = nullptr;
    float rangeSq = m_range * m_range;

    for (const Bloon* b : activeBloons) {
        if (!b || !b->active) continue;

        float dx = b->x - m_x;
        float dy = b->y - m_y;
        float distSq = dx * dx + dy * dy;

        if (distSq > rangeSq) {
            continue; // Out of range
        }

        if (!bestTarget) {
            bestTarget = b;
            continue;
        }

        switch (m_targetingMode) {
            case TargetingMode::First:
                if (b->distanceTraveled > bestTarget->distanceTraveled) {
                    bestTarget = b;
                }
                break;
            case TargetingMode::Last:
                if (b->distanceTraveled < bestTarget->distanceTraveled) {
                    bestTarget = b;
                }
                break;
            case TargetingMode::Close: {
                float bestDx = bestTarget->x - m_x;
                float bestDy = bestTarget->y - m_y;
                float bestDistSq = bestDx * bestDx + bestDy * bestDy;
                if (distSq < bestDistSq) {
                    bestTarget = b;
                }
                break;
            }
            case TargetingMode::Strong: {
                int bRBE = getBloonRBE(b->type);
                int bestRBE = getBloonRBE(bestTarget->type);
                if (bRBE > bestRBE || (bRBE == bestRBE && b->distanceTraveled > bestTarget->distanceTraveled)) {
                    bestTarget = b;
                }
                break;
            }
        }
    }

    return bestTarget;
}

bool Tower::attack(BloonPool& bloonPool, ProjectilePool& projectilePool, const Map& map) {
    (void)map;
    if (!canAttack()) {
        return false;
    }

    std::vector<Bloon*> activeBloons;
    bloonPool.getActiveBloons(activeBloons);

    if (m_type == TowerType::TackShooter) {
        // Check if ANY bloon is in range
        const Bloon* anyTarget = selectTarget(activeBloons);
        if (!anyTarget) {
            return false;
        }

        // Fire 8 tacks radially in 8 directions
        const float angles[8] = {
            0.0f, 0.785398f, 1.570796f, 2.356194f,
            3.141592f, 3.92699f, 4.712389f, 5.497787f
        };
        for (float angle : angles) {
            float dirX = std::cos(angle);
            float dirY = std::sin(angle);
            projectilePool.spawn(
                m_stats.projectileType,
                m_stats.damageType,
                m_x, m_y,
                dirX, dirY,
                m_stats.projectileSpeed,
                m_stats.projectileDamage,
                m_stats.projectilePierce,
                0.0f,
                0.35f // Short lifespan for tacks
            );
        }
        resetCooldown();
        return true;
    }

    const Bloon* target = selectTarget(activeBloons);
    if (!target) {
        return false;
    }

    float dirX = target->x - m_x;
    float dirY = target->y - m_y;

    projectilePool.spawn(
        m_stats.projectileType,
        m_stats.damageType,
        m_x, m_y,
        dirX, dirY,
        m_stats.projectileSpeed,
        m_stats.projectileDamage,
        m_stats.projectilePierce,
        m_stats.explosionRadius,
        1.5f
    );

    resetCooldown();
    return true;
}

} // namespace btd4
