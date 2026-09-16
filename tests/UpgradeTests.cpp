#include "TestRunner.hpp"
#include "game/Upgrade.hpp"

using namespace btd4;
using namespace btd4::test;

TEST_CASE(UpgradeParserLoadsDefinitions) {
    const std::string json = R"({
        "version": 1,
        "upgrades": [
            {"id":"dart_sharp_shots","tower":"DartMonkey","path":0,"tier":1,
             "displayName":"Sharp Shots","cost":100,"pierceAdd":1},
            {"id":"bomb_bigger","tower":"BombTower","path":0,"tier":1,
             "displayName":"Bigger Bombs","cost":250,"explosionRadiusAdd":8,"damageAdd":1}
        ]
    })";

    UpgradeSet upgrades;
    std::string error;
    TEST_ASSERT(parseUpgrades(json, upgrades, error));
    TEST_ASSERT_EQ(upgrades.upgrades.size(), static_cast<size_t>(2));

    const UpgradeDefinition* dart = findUpgrade(upgrades, TowerType::DartMonkey, 0, 1);
    TEST_ASSERT(dart != nullptr);
    TEST_ASSERT_EQ(dart->effect.cost, 100);
    TEST_ASSERT_EQ(dart->effect.pierceAdd, 1);
}

TEST_CASE(UpgradeValidationRejectsDuplicateTier) {
    UpgradeSet upgrades;
    UpgradeDefinition a;
    a.id = "a";
    a.displayName = "A";
    a.tower = TowerType::DartMonkey;
    a.path = 0;
    a.tier = 1;
    UpgradeDefinition b = a;
    b.id = "b";
    upgrades.upgrades = {a, b};

    std::string error;
    TEST_ASSERT(!validateUpgrades(upgrades, error));
    TEST_ASSERT(!error.empty());
}
