#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include "../map/Map.hpp"

namespace btd4 {

enum class BloonType : uint8_t {
    None = 0,
    Red,
    Blue,
    Green,
    Yellow,
    Pink,
    Black,
    White,
    Lead,
    Rainbow,
    Ceramic,
    MOAB
};

enum class DamageType : uint8_t {
    Sharp = 0,     // Regular darts, cannot pop Lead
    Explosive,     // Bombs, cannot pop Black
    Energy,        // Freeze/Energy, cannot pop White
    All            // Plasma, Juggernaut, pops all
};

struct BloonStats {
    int health{1};
    float speedMultiplier{1.0f};
    int r{255}, g{0}, b{0};
    float radius{7.0f};
    bool immuneToSharp{false};
    bool immuneToExplosive{false};
    bool immuneToEnergy{false};
};

BloonStats getBloonBaseStats(BloonType type);
std::vector<BloonType> getBloonChildren(BloonType type);
int getBloonRBE(BloonType type); // Red Bloon Equivalent (lives penalty on leak)

struct Bloon {
    uint32_t id{0};
    BloonType type{BloonType::None};
    int health{1};
    float speed{40.0f}; // Pixels per second
    float radius{7.0f};

    size_t pathIndex{0};
    float distanceTraveled{0.0f};
    float x{0.0f};
    float y{0.0f};

    bool active{false};
    bool popped{false};
    bool leaked{false};

    bool canTakeDamage(DamageType damageType) const;
    bool takeDamage(int amount, DamageType damageType, std::vector<BloonType>& outChildren);
};

class BloonPool {
public:
    static constexpr size_t MAX_BLOONS = 2048;

    BloonPool();

    void clear();

    Bloon* spawn(BloonType type, size_t pathIndex = 0, float initialDistance = 0.0f);
    void despawn(uint32_t id);

    void update(float deltaTime, const Map& map, std::vector<uint32_t>& outLeakedIds);

    // Pops a bloon, spawns children in the pool, and returns total cash earned (1 per layer popped)
    int damageBloon(uint32_t id, int damage, DamageType damageType, const Map& map);

    const std::array<Bloon, MAX_BLOONS>& allBloons() const { return m_pool; }
    size_t activeCount() const { return m_activeCount; }

    Bloon* findById(uint32_t id);
    const Bloon* findById(uint32_t id) const;

    // Get all active bloon pointers for targeting
    void getActiveBloons(std::vector<Bloon*>& outBloons);
    void getActiveBloons(std::vector<const Bloon*>& outBloons) const;

private:
    std::array<Bloon, MAX_BLOONS> m_pool{};
    size_t m_activeCount{0};
    uint32_t m_nextId{1};
};

} // namespace btd4
