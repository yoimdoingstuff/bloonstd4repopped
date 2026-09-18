#include "TestRunner.hpp"
#include "game/Rounds.hpp"
#include "game/GameState.hpp"
#include <limits>

namespace {
btd4::Map roundMap() {
    btd4::Map map("Original round test");
    map.addPath(btd4::Path({{10,20},{410,20}}));
    map.addPath(btd4::Path({{30,40},{430,40}}));
    return map;
}
btd4::RoundSet oneRound(btd4::BloonType type = btd4::BloonType::Red,
                       uint32_t count = 1, uint32_t spacing = 100, uint32_t delay = 0) {
    return {{{{{type, count, spacing, delay, 0}}}}};
}
void despawnAll(btd4::BloonPool& pool) {
    for (const auto& bloon : pool.allBloons()) if (bloon.active) pool.despawn(bloon.id);
}
const std::string roundJson = R"({"version":1,"rounds":[{"groups":[{"type":"red","count":3,"spacing_ms":100},{"type":"blue","count":2,"spacing_ms":50,"delay_ms":200,"path":1}]},{"groups":[{"type":"moab","count":1,"spacing_ms":0}]}]})";
struct RoundFiles : btd4::IFileSystem {
    bool available = true;
    bool readFile(const std::string&, std::vector<uint8_t>& out) const override {
        if (!available) return false;
        out.assign(roundJson.begin(), roundJson.end()); return true;
    }
    bool writeFile(const std::string&, const std::vector<uint8_t>&) override { return false; }
    bool fileExists(const std::string&) const override { return available; }
    bool createDirectories(const std::string&) override { return false; }
    bool removeFile(const std::string&) override { return false; }
    bool listFiles(const std::string&, std::vector<std::string>&) const override { return false; }
};
}
TEST_CASE(RoundLoaderValidatesAndReadsFiles) {
    auto map = roundMap();
    btd4::RoundSet rounds;
    std::string error;
    RoundFiles files;
    TEST_ASSERT(btd4::loadRounds(files, "rounds.json", map, rounds, error));
    TEST_ASSERT(error.empty());
    TEST_ASSERT_EQ(rounds.rounds.size(), size_t(2));
    TEST_ASSERT_EQ(rounds.rounds[0].groups[1].pathIndex, size_t(1));
    TEST_ASSERT_EQ(rounds.rounds[1].groups[0].type, btd4::BloonType::MOAB);
    files.available = false;
    TEST_ASSERT(!btd4::loadRounds(files, "missing.json", map, rounds, error));
    TEST_ASSERT(!error.empty());
    TEST_ASSERT_EQ(rounds.rounds.size(), size_t(2));
    const std::vector<std::string> invalid = {
        "", "{}", roundJson + "x", R"({"version":2,"rounds":[]})",
        R"({"version":1,"rounds":[]})",
        R"({"version":1,"rounds":[{"groups":[]}]})",
        R"({"version":1,"version":1,"rounds":[]})",
        std::string(1024*1024+1, ' ')
    };
    for (const auto& json : invalid) {
        TEST_ASSERT(!btd4::parseRounds(json, map, rounds, error));
        TEST_ASSERT_EQ(rounds.rounds.size(), size_t(2));
        TEST_ASSERT(!error.empty());
    }
    for (const auto& group : {
        R"({"type":"unknown","count":1,"spacing_ms":0})",
        R"({"type":"red","count":0,"spacing_ms":0})",
        R"({"type":"red","count":-1,"spacing_ms":0})",
        R"({"type":"red","count":1.5,"spacing_ms":0})",
        R"({"type":"red","count":100001,"spacing_ms":0})",
        R"({"type":"red","count":3,"spacing_ms":3600000})",
        R"({"type":"red","count":1,"spacing_ms":0,"path":2})",
        R"({"type":"red","count":1,"spacing_ms":0,"count":2})",
        R"({"type":"red","count":1,"spacing_ms":0,"typo":0})",
        R"({"type":"red","count":1})"
    }) {
        TEST_ASSERT(!btd4::parseRounds(std::string(R"({"version":1,"rounds":[{"groups":[)") + group + "]}]}", map, rounds, error));
        TEST_ASSERT_EQ(rounds.rounds.size(), size_t(2));
    }
    btd4::Map empty;
    TEST_ASSERT(!btd4::parseRounds(roundJson, empty, rounds, error));
}
TEST_CASE(RoundsScheduleSequentialGroupsAtTheirPathOrigins) {
    auto map = roundMap();
    btd4::RoundSet rounds;
    btd4::RoundScheduler scheduler;
    btd4::BloonPool pool;
    std::string error;
    TEST_ASSERT(btd4::parseRounds(roundJson, map, rounds, error));
    TEST_ASSERT(scheduler.configure(rounds, map, error));
    TEST_ASSERT(scheduler.start(pool, map));
    TEST_ASSERT_EQ(pool.activeCount(), size_t(1));
    TEST_ASSERT_EQ(pool.findById(1)->x, 10.0f);
    TEST_ASSERT(!scheduler.start(pool, map));
    scheduler.advance(0.099, pool, map);
    TEST_ASSERT_EQ(pool.activeCount(), size_t(1));
    scheduler.advance(0.001, pool, map);
    TEST_ASSERT_EQ(pool.activeCount(), size_t(2));
    scheduler.advance(0.299, pool, map);
    TEST_ASSERT_EQ(pool.activeCount(), size_t(3));
    scheduler.advance(0.001, pool, map);
    TEST_ASSERT_EQ(pool.activeCount(), size_t(4));
    TEST_ASSERT_EQ(pool.findById(4)->type, btd4::BloonType::Blue);
    TEST_ASSERT_EQ(pool.findById(4)->x, 30.0f);
    scheduler.advance(0.05, pool, map);
    TEST_ASSERT_EQ(pool.activeCount(), size_t(5));
    TEST_ASSERT(scheduler.active());
    despawnAll(pool);
    TEST_ASSERT(scheduler.advance(0, pool, map));
    TEST_ASSERT_EQ(scheduler.completedRounds(), size_t(1));
    TEST_ASSERT(!scheduler.advance(1, pool, map));
    TEST_ASSERT(!scheduler.finished());
}
TEST_CASE(RoundsRetainSpawnsWhenPoolIsFull) {
    auto map = roundMap();
    btd4::RoundScheduler scheduler;
    btd4::BloonPool pool;
    std::string error;
    TEST_ASSERT(scheduler.configure(oneRound(btd4::BloonType::Red, 2050, 0), map, error));
    TEST_ASSERT(scheduler.start(pool, map));
    TEST_ASSERT_EQ(pool.activeCount(), size_t(2048));
    pool.despawn(1);
    scheduler.advance(1, pool, map);
    TEST_ASSERT_EQ(pool.activeCount(), size_t(2048));
    TEST_ASSERT(pool.findById(2049) != nullptr);
    despawnAll(pool);
    TEST_ASSERT(!scheduler.advance(0, pool, map));
    TEST_ASSERT_EQ(pool.activeCount(), size_t(1));
    TEST_ASSERT(pool.findById(2050) != nullptr);
    despawnAll(pool);
    TEST_ASSERT(scheduler.advance(0, pool, map));
    TEST_ASSERT(scheduler.finished());
}
TEST_CASE(RoundScheduleIgnoresInvalidTimeAndMatchesTickPartitions) {
    auto map = roundMap();
    btd4::RoundScheduler a, b;
    btd4::BloonPool poolA, poolB;
    std::string error;
    TEST_ASSERT(a.configure(oneRound(btd4::BloonType::Red, 11, 100), map, error));
    TEST_ASSERT(b.configure(oneRound(btd4::BloonType::Red, 11, 100), map, error));
    a.start(poolA, map); b.start(poolB, map);
    a.advance(-1, poolA, map);
    a.advance(std::numeric_limits<double>::infinity(), poolA, map);
    a.advance(std::numeric_limits<double>::quiet_NaN(), poolA, map);
    TEST_ASSERT_EQ(poolA.activeCount(), size_t(1));
    a.advance(1, poolA, map);
    for (int i = 0; i < 60; ++i) b.advance(1.0/60.0, poolB, map);
    TEST_ASSERT_EQ(poolA.activeCount(), size_t(11));
    TEST_ASSERT_EQ(poolB.activeCount(), poolA.activeCount());
}
TEST_CASE(SimulationRoundsPauseWaitForChildrenRewardAndReset) {
    btd4::GameSimulation sim(roundMap());
    std::string error;
    auto rounds = oneRound(btd4::BloonType::Blue);
    rounds.rounds.push_back(oneRound().rounds.front());
    TEST_ASSERT(sim.setRounds(rounds, error));
    TEST_ASSERT(sim.startNextRound());
    TEST_ASSERT(!sim.setRounds(rounds, error));
    sim.pause();
    sim.update(10);
    TEST_ASSERT_EQ(sim.bloonPool().findById(1)->distanceTraveled, 0.0f);
    TEST_ASSERT(!sim.startNextRound());
    sim.resume();
    sim.bloonPool().damageBloon(1, 1, btd4::DamageType::All, sim.map());
    sim.update(0.01f);
    TEST_ASSERT(sim.roundActive()); // The blue's red child is still alive.
    TEST_ASSERT_EQ(sim.economy().cash(), 650);
    despawnAll(sim.bloonPool());
    sim.update(0.01f);
    TEST_ASSERT(!sim.roundActive());
    TEST_ASSERT_EQ(sim.completedRounds(), size_t(1));
    TEST_ASSERT_EQ(sim.economy().cash(), 751);
    sim.update(1);
    TEST_ASSERT_EQ(sim.economy().cash(), 751);
    TEST_ASSERT(sim.startNextRound());
    TEST_ASSERT_EQ(sim.currentRound(), 2);
    despawnAll(sim.bloonPool());
    sim.update(0.01f);
    TEST_ASSERT_EQ(sim.economy().cash(), 853);
    TEST_ASSERT_EQ(sim.state(), btd4::GameStateType::Victory);
    TEST_ASSERT(!sim.startNextRound());
    sim.update(1);
    TEST_ASSERT_EQ(sim.economy().cash(), 853);
    sim.reset();
    TEST_ASSERT_EQ(sim.economy().cash(), 650);
    TEST_ASSERT_EQ(sim.completedRounds(), size_t(0));
    TEST_ASSERT(sim.startNextRound());
    TEST_ASSERT_EQ(sim.bloonPool().findById(1)->type, btd4::BloonType::Blue);
}
TEST_CASE(SimulationDefeatDoesNotAwardRoundReward) {
    btd4::GameSimulation sim(roundMap());
    std::string error;
    TEST_ASSERT(sim.setRounds(oneRound(), error));
    sim.economy().reset(650, 1);
    TEST_ASSERT(sim.startNextRound());
    sim.update(11);
    TEST_ASSERT_EQ(sim.state(), btd4::GameStateType::GameOver);
    TEST_ASSERT_EQ(sim.economy().cash(), 650);
    TEST_ASSERT_EQ(sim.completedRounds(), size_t(0));
    TEST_ASSERT(!sim.startNextRound());
}
TEST_CASE(SimulationDelayedSpawnDoesNotMoveBeforeBirth) {
    btd4::GameSimulation sim(roundMap());
    std::string error;
    TEST_ASSERT(sim.setRounds(oneRound(btd4::BloonType::Red, 1, 0, 5), error));
    TEST_ASSERT(sim.startNextRound());
    TEST_ASSERT_EQ(sim.bloonPool().activeCount(), size_t(0));
    sim.update(0.01f);
    TEST_ASSERT_EQ(sim.bloonPool().activeCount(), size_t(1));
    TEST_ASSERT_EQ(sim.bloonPool().findById(1)->distanceTraveled, 0.0f);
    TEST_ASSERT_EQ(sim.bloonPool().findById(1)->x, 10.0f);
    TEST_ASSERT(sim.roundActive());
    sim.update(0.01f);
    TEST_ASSERT(sim.bloonPool().findById(1)->distanceTraveled > 0);
}


TEST_CASE(RoundSerializationRoundTripsEditorData) {
    btd4::Map map("Round Test Map");
    map.addPath(btd4::Path({{0.0f, 0.0f}, {200.0f, 0.0f}}));

    btd4::RoundSet source;
    btd4::RoundDefinition round;
    round.groups.push_back({btd4::BloonType::Blue, 15, 420, 250, 0});
    round.groups.push_back({btd4::BloonType::Lead, 3, 900, 1000, 0});
    source.rounds.push_back(round);

    const std::string json = btd4::serializeRounds(source);
    btd4::RoundSet loaded;
    std::string error;
    TEST_ASSERT(btd4::parseRounds(json, map, loaded, error));
    TEST_ASSERT(error.empty());
    TEST_ASSERT_EQ(loaded.rounds.size(), size_t(1));
    TEST_ASSERT_EQ(loaded.rounds[0].groups.size(), size_t(2));
    TEST_ASSERT_EQ(loaded.rounds[0].groups[0].count, uint32_t(15));
    TEST_ASSERT_EQ(loaded.rounds[0].groups[1].type, btd4::BloonType::Lead);
    TEST_ASSERT_EQ(loaded.rounds[0].groups[1].delayMs, uint32_t(1000));
}
