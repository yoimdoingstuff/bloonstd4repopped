#include "TestRunner.hpp"
#include "game/Freeplay.hpp"
#include "map/Map.hpp"

using namespace btd4;

namespace {
Map testMap() {
    Map map;
    map.setName("freeplay-test");
    Path path;
    path.addWaypoint(0.0f, 0.0f);
    path.addWaypoint(400.0f, 0.0f);
    path.addWaypoint(800.0f, 0.0f);
    map.paths().push_back(path);
    return map;
}
}

TEST_CASE(FreeplayGeneratorRejectsInvalidRound) {
    FreeplayGenerator generator;
    RoundSet campaign;
    FreeplayRound output;
    TEST_ASSERT(!generator.generate(0, campaign, output));
}

TEST_CASE(FreeplayGeneratorCyclesCampaignRounds) {
    Map map = testMap();
    RoundSet campaign;
    campaign.rounds.resize(2);
    campaign.rounds[0].groups.push_back({BloonType::Red, 5, 100, 0, 0});
    campaign.rounds[1].groups.push_back({BloonType::Blue, 10, 200, 0, 0});

    std::string error;
    TEST_ASSERT(validateRounds(campaign, map, error));

    FreeplayGenerator generator({1, 100, 0.02f, 0.005f, 25});
    FreeplayRound first;
    FreeplayRound third;
    TEST_ASSERT(generator.generate(1, campaign, first));
    TEST_ASSERT(generator.generate(3, campaign, third));
    TEST_ASSERT_EQ(first.definition.groups.front().type, BloonType::Red);
    TEST_ASSERT_EQ(third.definition.groups.front().type, BloonType::Red);
    TEST_ASSERT(third.healthScale > first.healthScale);
}

TEST_CASE(FreeplayGeneratorScalesSpawnPressure) {
    Map map = testMap();
    RoundSet campaign;
    campaign.rounds.resize(1);
    campaign.rounds[0].groups.push_back({BloonType::Red, 10, 1000, 0, 0});

    FreeplayGenerator generator({1, 100, 0.02f, 0.10f, 25});
    FreeplayRound early;
    FreeplayRound late;
    TEST_ASSERT(generator.generate(1, campaign, early));
    TEST_ASSERT(generator.generate(11, campaign, late));
    TEST_ASSERT(late.definition.groups.front().count > early.definition.groups.front().count);
    TEST_ASSERT(late.definition.groups.front().spacingMs < early.definition.groups.front().spacingMs);
}
