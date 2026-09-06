#pragma once
#include "Bloon.hpp"
#include "core/IFileSystem.hpp"
#include <string_view>

namespace btd4 {
struct BloonGroup {
    BloonType type{BloonType::Red};
    uint32_t count{1};
    uint32_t spacingMs{1000};
    uint32_t delayMs{0};
    size_t pathIndex{0};
};
struct RoundDefinition { std::vector<BloonGroup> groups; };
struct RoundSet { std::vector<RoundDefinition> rounds; };

// Includes map path validation. Failure leaves output unchanged.
bool validateRounds(const RoundSet& rounds, const Map& map, std::string& error);
bool parseRounds(std::string_view json, const Map& map, RoundSet& output, std::string& error);
bool loadRounds(const IFileSystem& files, const std::string& path, const Map& map,
                RoundSet& output, std::string& error);

// Allocation-free scheduling after configuration. The map must remain unchanged
// during an active round. Spawns occur at the first simulation boundary due.
class RoundScheduler {
public:
    bool configure(RoundSet rounds, const Map& map, std::string& error);
    void reset();
    bool start(BloonPool& pool, const Map& map);
    // Returns true once per completed round, never just for finishing spawning.
    bool advance(double seconds, BloonPool& pool, const Map& map);
    bool active() const { return m_active; }
    bool finished() const { return !m_rounds.rounds.empty() && m_nextRound == m_rounds.rounds.size(); }
    size_t completedRounds() const { return m_nextRound; }
    size_t roundCount() const { return m_rounds.rounds.size(); }
private:
    RoundSet m_rounds;
    size_t m_nextRound{0}, m_group{0};
    uint32_t m_spawned{0};
    uint64_t m_dueMs{0};
    double m_elapsedMs{0};
    bool m_active{false};
};
}
