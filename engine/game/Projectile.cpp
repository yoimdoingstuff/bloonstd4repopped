#include "Projectile.hpp"
#include <algorithm>
#include <cmath>

namespace btd4 {

bool Projectile::hasHitBloon(uint32_t bloonId) const {
    for (uint32_t id : hitBloonIds) {
        if (id == bloonId) return true;
    }
    return false;
}

void Projectile::recordHit(uint32_t bloonId) {
    hitBloonIds.push_back(bloonId);
}

ProjectilePool::ProjectilePool() {
    clear();
}

void ProjectilePool::clear() {
    for (auto& p : m_pool) {
        p.active = false;
        p.id = 0;
        p.hitBloonIds.clear();
    }
    m_activeCount = 0;
    m_nextId = 1;
}

Projectile* ProjectilePool::spawn(
    ProjectileType type,
    DamageType damageType,
    float x, float y,
    float dirX, float dirY,
    float speed,
    int damage,
    int pierce,
    float explosionRadius,
    float lifespan
) {
    float len = std::sqrt(dirX * dirX + dirY * dirY);
    float normX = (len > 0.0001f) ? (dirX / len) : 1.0f;
    float normY = (len > 0.0001f) ? (dirY / len) : 0.0f;

    for (auto& p : m_pool) {
        if (!p.active) {
            p.id = m_nextId++;
            p.type = type;
            p.damageType = damageType;
            p.x = x;
            p.y = y;
            p.vx = normX * speed;
            p.vy = normY * speed;
            p.radius = 4.0f;
            p.damage = damage;
            p.pierce = pierce;
            p.explosionRadius = explosionRadius;
            p.lifespan = lifespan;
            p.active = true;
            p.hitBloonIds.clear();
            m_activeCount++;
            return &p;
        }
    }
    return nullptr;
}

void ProjectilePool::despawn(uint32_t id) {
    for (auto& p : m_pool) {
        if (p.active && p.id == id) {
            p.active = false;
            p.hitBloonIds.clear();
            m_activeCount--;
            return;
        }
    }
}

int ProjectilePool::update(float deltaTime, BloonPool& bloonPool, const Map& map) {
    int totalCashEarned = 0;

    std::vector<Bloon*> activeBloons;
    bloonPool.getActiveBloons(activeBloons);

    for (auto& p : m_pool) {
        if (!p.active) {
            continue;
        }

        p.lifespan -= deltaTime;
        if (p.lifespan <= 0.0f) {
            p.active = false;
            p.hitBloonIds.clear();
            m_activeCount--;
            continue;
        }

        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;

        // Check bounds (screen margin)
        if (p.x < -20.0f || p.x > 500.0f || p.y < -20.0f || p.y > 300.0f) {
            p.active = false;
            p.hitBloonIds.clear();
            m_activeCount--;
            continue;
        }

        // Check collision against active bloons
        for (Bloon* b : activeBloons) {
            if (!b->active || p.hasHitBloon(b->id)) {
                continue;
            }

            float dx = p.x - b->x;
            float dy = p.y - b->y;
            float combinedRadius = p.radius + b->radius;

            if ((dx * dx + dy * dy) <= (combinedRadius * combinedRadius)) {
                // Collision confirmed
                p.recordHit(b->id);

                if (p.explosionRadius > 0.0f) {
                    // Area of effect explosion
                    float expRadiusSq = p.explosionRadius * p.explosionRadius;
                    for (Bloon* target : activeBloons) {
                        if (!target->active) continue;
                        float edx = p.x - target->x;
                        float edy = p.y - target->y;
                        if ((edx * edx + edy * edy) <= expRadiusSq) {
                            totalCashEarned += bloonPool.damageBloon(target->id, p.damage, p.damageType, map);
                        }
                    }
                    p.pierce = 0; // Bombs explode and disappear
                } else {
                    // Direct hit
                    totalCashEarned += bloonPool.damageBloon(b->id, p.damage, p.damageType, map);
                    p.pierce--;
                }

                if (p.pierce <= 0) {
                    p.active = false;
                    p.hitBloonIds.clear();
                    m_activeCount--;
                    break;
                }
            }
        }
    }

    return totalCashEarned;
}

} // namespace btd4
