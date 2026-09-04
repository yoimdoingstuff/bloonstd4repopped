#include "Bloon.hpp"
#include <algorithm>

namespace btd4 {

static constexpr float BASE_BLOON_SPEED = 40.0f; // Pixels per second for red bloon

BloonStats getBloonBaseStats(BloonType type) {
    BloonStats stats;
    switch (type) {
        case BloonType::Red:
            stats.health = 1;
            stats.speedMultiplier = 1.0f;
            stats.r = 255; stats.g = 0; stats.b = 0;
            stats.radius = 7.0f;
            break;
        case BloonType::Blue:
            stats.health = 1;
            stats.speedMultiplier = 1.4f;
            stats.r = 0; stats.g = 120; stats.b = 255;
            stats.radius = 7.5f;
            break;
        case BloonType::Green:
            stats.health = 1;
            stats.speedMultiplier = 1.8f;
            stats.r = 30; stats.g = 200; stats.b = 30;
            stats.radius = 8.0f;
            break;
        case BloonType::Yellow:
            stats.health = 1;
            stats.speedMultiplier = 3.2f;
            stats.r = 255; stats.g = 230; stats.b = 0;
            stats.radius = 8.5f;
            break;
        case BloonType::Pink:
            stats.health = 1;
            stats.speedMultiplier = 3.5f;
            stats.r = 255; stats.g = 100; stats.b = 180;
            stats.radius = 8.5f;
            break;
        case BloonType::Black:
            stats.health = 1;
            stats.speedMultiplier = 1.8f;
            stats.r = 20; stats.g = 20; stats.b = 20;
            stats.radius = 7.0f;
            stats.immuneToExplosive = true;
            break;
        case BloonType::White:
            stats.health = 1;
            stats.speedMultiplier = 2.0f;
            stats.r = 240; stats.g = 240; stats.b = 255;
            stats.radius = 7.0f;
            stats.immuneToEnergy = true;
            break;
        case BloonType::Lead:
            stats.health = 1;
            stats.speedMultiplier = 1.0f;
            stats.r = 130; stats.g = 130; stats.b = 140;
            stats.radius = 8.0f;
            stats.immuneToSharp = true;
            break;
        case BloonType::Rainbow:
            stats.health = 1;
            stats.speedMultiplier = 2.2f;
            stats.r = 255; stats.g = 180; stats.b = 50;
            stats.radius = 9.0f;
            break;
        case BloonType::Ceramic:
            stats.health = 10;
            stats.speedMultiplier = 2.5f;
            stats.r = 160; stats.g = 90; stats.b = 40;
            stats.radius = 9.5f;
            break;
        case BloonType::MOAB:
            stats.health = 200;
            stats.speedMultiplier = 1.0f;
            stats.r = 0; stats.g = 80; stats.b = 220;
            stats.radius = 20.0f;
            break;
        default:
            break;
    }
    return stats;
}

std::vector<BloonType> getBloonChildren(BloonType type) {
    switch (type) {
        case BloonType::Blue:
            return {BloonType::Red};
        case BloonType::Green:
            return {BloonType::Blue};
        case BloonType::Yellow:
            return {BloonType::Green};
        case BloonType::Pink:
            return {BloonType::Yellow};
        case BloonType::Black:
        case BloonType::White:
            return {BloonType::Pink, BloonType::Pink};
        case BloonType::Lead:
            return {BloonType::Black, BloonType::Black};
        case BloonType::Rainbow:
            return {BloonType::Black, BloonType::Black, BloonType::White, BloonType::White};
        case BloonType::Ceramic:
            return {BloonType::Rainbow, BloonType::Rainbow};
        case BloonType::MOAB:
            return {BloonType::Ceramic, BloonType::Ceramic, BloonType::Ceramic, BloonType::Ceramic};
        default:
            return {};
    }
}

int getBloonRBE(BloonType type) {
    switch (type) {
        case BloonType::Red:     return 1;
        case BloonType::Blue:    return 2;
        case BloonType::Green:   return 3;
        case BloonType::Yellow:  return 4;
        case BloonType::Pink:    return 5;
        case BloonType::Black:   return 11;
        case BloonType::White:   return 11;
        case BloonType::Lead:    return 23;
        case BloonType::Rainbow: return 47;
        case BloonType::Ceramic: return 104;
        case BloonType::MOAB:    return 616;
        default:                 return 0;
    }
}

bool Bloon::canTakeDamage(DamageType damageType) const {
    BloonStats stats = getBloonBaseStats(type);
    if (damageType == DamageType::All) {
        return true;
    }
    if (damageType == DamageType::Sharp && stats.immuneToSharp) {
        return false;
    }
    if (damageType == DamageType::Explosive && stats.immuneToExplosive) {
        return false;
    }
    if (damageType == DamageType::Energy && stats.immuneToEnergy) {
        return false;
    }
    return true;
}

bool Bloon::takeDamage(int amount, DamageType damageType, std::vector<BloonType>& outChildren) {
    outChildren.clear();
    if (!active || popped || !canTakeDamage(damageType)) {
        return false;
    }

    health -= amount;
    if (health <= 0) {
        popped = true;
        outChildren = getBloonChildren(type);
    }
    return true;
}

BloonPool::BloonPool() {
    clear();
}

void BloonPool::clear() {
    for (auto& b : m_pool) {
        b.active = false;
        b.popped = false;
        b.leaked = false;
        b.id = 0;
    }
    m_activeCount = 0;
    m_nextId = 1;
}

Bloon* BloonPool::spawn(BloonType type, size_t pathIndex, float initialDistance) {
    if (type == BloonType::None) {
        return nullptr;
    }

    // Find free slot
    for (auto& b : m_pool) {
        if (!b.active) {
            b.id = m_nextId++;
            b.type = type;
            BloonStats stats = getBloonBaseStats(type);
            b.health = stats.health;
            b.speed = BASE_BLOON_SPEED * stats.speedMultiplier;
            b.radius = stats.radius;
            b.pathIndex = pathIndex;
            b.distanceTraveled = initialDistance;
            b.active = true;
            b.popped = false;
            b.leaked = false;
            m_activeCount++;
            return &b;
        }
    }
    return nullptr; // Pool full
}

void BloonPool::despawn(uint32_t id) {
    for (auto& b : m_pool) {
        if (b.active && b.id == id) {
            b.active = false;
            b.popped = false;
            b.leaked = false;
            m_activeCount--;
            return;
        }
    }
}

void BloonPool::update(float deltaTime, const Map& map, std::vector<uint32_t>& outLeakedIds) {
    outLeakedIds.clear();

    const auto& paths = map.paths();
    for (auto& b : m_pool) {
        if (!b.active || b.popped || b.leaked) {
            continue;
        }

        if (b.pathIndex >= paths.size()) {
            continue;
        }

        const Path& path = paths[b.pathIndex];
        b.distanceTraveled += b.speed * deltaTime;

        Point2D pos = path.getPositionAtDistance(b.distanceTraveled);
        b.x = pos.x;
        b.y = pos.y;

        if (path.isAtEnd(b.distanceTraveled)) {
            b.leaked = true;
            b.active = false;
            m_activeCount--;
            outLeakedIds.push_back(b.id);
        }
    }
}

int BloonPool::damageBloon(uint32_t id, int damage, DamageType damageType, const Map& map) {
    Bloon* target = findById(id);
    if (!target || !target->active) {
        return 0;
    }

    std::vector<BloonType> children;
    float dist = target->distanceTraveled;
    size_t pathIdx = target->pathIndex;

    bool hit = target->takeDamage(damage, damageType, children);
    if (!hit) {
        return 0;
    }

    int cashEarned = 1; // 1 cash per damage dealt / layer popped

    if (target->popped) {
        target->active = false;
        m_activeCount--;

        // Spawn child bloons slightly staggered along path to avoid exact overlap
        float offset = 0.0f;
        for (BloonType childType : children) {
            Bloon* child = spawn(childType, pathIdx, std::max(0.0f, dist - offset));
            if (child && pathIdx < map.paths().size()) {
                Point2D p = map.paths()[pathIdx].getPositionAtDistance(child->distanceTraveled);
                child->x = p.x;
                child->y = p.y;
            }
            offset += 3.0f;
        }
    }

    return cashEarned;
}

Bloon* BloonPool::findById(uint32_t id) {
    for (auto& b : m_pool) {
        if (b.active && b.id == id) {
            return &b;
        }
    }
    return nullptr;
}

const Bloon* BloonPool::findById(uint32_t id) const {
    for (const auto& b : m_pool) {
        if (b.active && b.id == id) {
            return &b;
        }
    }
    return nullptr;
}

void BloonPool::getActiveBloons(std::vector<Bloon*>& outBloons) {
    outBloons.clear();
    outBloons.reserve(m_activeCount);
    for (auto& b : m_pool) {
        if (b.active && !b.popped && !b.leaked) {
            outBloons.push_back(&b);
        }
    }
}

void BloonPool::getActiveBloons(std::vector<const Bloon*>& outBloons) const {
    outBloons.clear();
    outBloons.reserve(m_activeCount);
    for (const auto& b : m_pool) {
        if (b.active && !b.popped && !b.leaked) {
            outBloons.push_back(&b);
        }
    }
}

} // namespace btd4
