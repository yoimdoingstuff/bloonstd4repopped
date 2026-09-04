#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include "Bloon.hpp"

namespace btd4 {

enum class ProjectileType : uint8_t {
    Dart = 0,
    Tack,
    Bomb,
    Boomerang,
    SniperShot,
    Laser,
    Plasma
};

struct Projectile {
    uint32_t id{0};
    ProjectileType type{ProjectileType::Dart};
    DamageType damageType{DamageType::Sharp};

    float x{0.0f};
    float y{0.0f};
    float vx{0.0f};
    float vy{0.0f};
    float radius{4.0f};

    int damage{1};
    int pierce{1};
    float explosionRadius{0.0f}; // 0 = single target, >0 = AoE explosion
    float lifespan{2.0f};

    bool active{false};
    std::vector<uint32_t> hitBloonIds;

    bool hasHitBloon(uint32_t bloonId) const;
    void recordHit(uint32_t bloonId);
};

class ProjectilePool {
public:
    static constexpr size_t MAX_PROJECTILES = 1024;

    ProjectilePool();

    void clear();

    Projectile* spawn(
        ProjectileType type,
        DamageType damageType,
        float x, float y,
        float dirX, float dirY,
        float speed,
        int damage = 1,
        int pierce = 1,
        float explosionRadius = 0.0f,
        float lifespan = 2.0f
    );

    void despawn(uint32_t id);

    // Updates positions and handles collisions with bloons in BloonPool.
    // Accumulates and returns cash earned from hits/pops.
    int update(float deltaTime, BloonPool& bloonPool, const Map& map);

    size_t activeCount() const { return m_activeCount; }
    const std::array<Projectile, MAX_PROJECTILES>& allProjectiles() const { return m_pool; }

private:
    std::array<Projectile, MAX_PROJECTILES> m_pool{};
    size_t m_activeCount{0};
    uint32_t m_nextId{1};
};

} // namespace btd4
