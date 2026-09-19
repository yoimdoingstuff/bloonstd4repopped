#include "TestRunner.hpp"
#include "game/TowerData.hpp"
#include "game/Player.hpp"
#include "game/MultiplayerSession.hpp"
#include "game/GameState.hpp"
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
    a.id = "a"; a.displayName = "A"; a.tower = TowerType::DartMonkey; a.path = 0; a.tier = 1;
    UpgradeDefinition b = a; b.id = "b";
    upgrades.upgrades = {a, b};
    std::string error;
    TEST_ASSERT(!validateUpgrades(upgrades, error));
    TEST_ASSERT(!error.empty());
}

TEST_CASE(TowerAppliesUpgradeEffect) {
    Tower tower(1, TowerType::DartMonkey, 10.0f, 10.0f);
    const float baseRange = tower.range();
    const float baseCooldown = tower.attackCooldown();
    const int baseDamage = tower.projectileDamage();
    const int baseCost = tower.totalInvestedCost();

    UpgradeEffect effect;
    effect.cost = 125;
    effect.rangeAdd = 15.0f;
    effect.cooldownMultiplier = 0.8f;
    effect.damageAdd = 1;
    effect.pierceAdd = 2;
    effect.projectileSpeedMultiplier = 1.25f;

    TEST_ASSERT(tower.applyUpgrade(effect, 0, 1));
    TEST_ASSERT_EQ(tower.upgradeTier(0), static_cast<uint8_t>(1));
    TEST_ASSERT(tower.hasUpgrade(0, 1));
    TEST_ASSERT(tower.range() > baseRange);
    TEST_ASSERT(tower.attackCooldown() < baseCooldown);
    TEST_ASSERT_EQ(tower.projectileDamage(), baseDamage + 1);
    TEST_ASSERT_EQ(tower.totalInvestedCost(), baseCost + 125);
}

TEST_CASE(TowerRejectsSkippedUpgradeTier) {
    Tower tower(2, TowerType::TackShooter, 0.0f, 0.0f);
    UpgradeEffect effect;
    effect.cost = 100;
    TEST_ASSERT(!tower.applyUpgrade(effect, 0, 2));
    TEST_ASSERT_EQ(tower.upgradeTier(0), static_cast<uint8_t>(0));
}

TEST_CASE(TowerUsesSequentialBTD4UpgradeLevels) {
    Tower tower(3, TowerType::BombTower, 0.0f, 0.0f);
    UpgradeEffect effect; effect.cost = 100; effect.damageAdd = 1;

    TEST_ASSERT(!tower.applyUpgrade(effect, 1, 1));
    TEST_ASSERT(tower.applyUpgrade(effect, 0, 1));
    TEST_ASSERT(tower.applyUpgrade(effect, 0, 2));
    TEST_ASSERT(tower.applyUpgrade(effect, 0, 3));
    TEST_ASSERT(tower.applyUpgrade(effect, 0, 4));
    TEST_ASSERT(!tower.applyUpgrade(effect, 0, 5));
    TEST_ASSERT_EQ(tower.upgradeLevel(), static_cast<uint8_t>(4));
    TEST_ASSERT_EQ(tower.totalInvestedCost(), 700 + 400);
}


TEST_CASE(TowerDataSerializationRoundTripsBaseStats) {
    btd4::TowerSet source;
    btd4::TowerDefinition dart;
    dart.id = "dart_custom";
    dart.displayName = "Custom Dart";
    dart.type = btd4::TowerType::DartMonkey;
    dart.stats.cost = 321;
    dart.stats.range = 123.0f;
    dart.stats.attackCooldown = 0.75f;
    dart.stats.footprintRadius = 13.0f;
    dart.stats.projectileType = btd4::ProjectileType::Dart;
    dart.stats.damageType = btd4::DamageType::Sharp;
    dart.stats.projectileDamage = 2;
    dart.stats.projectilePierce = 4;
    dart.stats.projectileSpeed = 300.0f;
    source.towers.push_back(dart);

    const std::string json = btd4::serializeTowers(source);
    btd4::TowerSet loaded;
    std::string error;
    TEST_ASSERT(btd4::parseTowers(json, loaded, error));
    TEST_ASSERT(error.empty());
    TEST_ASSERT_EQ(loaded.towers.size(), size_t(1));
    TEST_ASSERT_EQ(loaded.towers[0].id, "dart_custom");
    TEST_ASSERT_EQ(loaded.towers[0].stats.cost, 321);
    TEST_ASSERT_EQ(loaded.towers[0].stats.projectilePierce, 4);
    TEST_ASSERT_EQ(loaded.towers[0].stats.projectileSpeed, 300.0f);
}

TEST_CASE(TowerDefinitionsOverrideBuiltinStats) {
    btd4::TowerSet definitions;
    btd4::TowerDefinition dart;
    dart.id = "dart_override";
    dart.displayName = "Override";
    dart.type = btd4::TowerType::DartMonkey;
    dart.stats = btd4::getTowerBaseStats(btd4::TowerType::DartMonkey);
    dart.stats.cost = 999;
    definitions.towers.push_back(dart);

    btd4::configureTowerDefinitions(definitions);
    TEST_ASSERT_EQ(btd4::getTowerBaseStats(btd4::TowerType::DartMonkey).cost, 999);
    btd4::configureTowerDefinitions(btd4::TowerSet{});
}


TEST_CASE(UpgradeSerializationRoundTripsEditorData) {
    btd4::UpgradeSet source;
    btd4::UpgradeDefinition upgrade;
    upgrade.id = "custom_shots";
    upgrade.tower = btd4::TowerType::DartMonkey;
    upgrade.path = 0;
    upgrade.tier = 2;
    upgrade.displayName = "Custom Shots";
    upgrade.effect.cost = 333;
    upgrade.effect.rangeAdd = 12.5f;
    upgrade.effect.cooldownMultiplier = 0.75f;
    upgrade.effect.damageAdd = 2;
    upgrade.effect.pierceAdd = 3;
    upgrade.effect.projectileSpeedMultiplier = 1.25f;
    upgrade.effect.explosionRadiusAdd = 4.0f;
    source.upgrades.push_back(upgrade);

    const std::string json = btd4::serializeUpgrades(source);
    btd4::UpgradeSet loaded;
    std::string error;
    TEST_ASSERT(btd4::parseUpgrades(json, loaded, error));
    TEST_ASSERT(error.empty());
    TEST_ASSERT_EQ(loaded.upgrades.size(), size_t(1));
    TEST_ASSERT_EQ(loaded.upgrades[0].displayName, "Custom Shots");
    TEST_ASSERT_EQ(loaded.upgrades[0].path, static_cast<uint8_t>(0));
    TEST_ASSERT_EQ(loaded.upgrades[0].effect.cost, 333);
}


TEST_CASE(TowerCyclesTargetingBothDirections) {
    btd4::Tower tower(4, btd4::TowerType::DartMonkey, 0.0f, 0.0f);
    TEST_ASSERT_EQ(tower.targetingMode(), btd4::TargetingMode::First);
    tower.cycleTargetingMode();
    TEST_ASSERT_EQ(tower.targetingMode(), btd4::TargetingMode::Last);
    tower.cycleTargetingMode();
    TEST_ASSERT_EQ(tower.targetingMode(), btd4::TargetingMode::Close);
    tower.cycleTargetingModeBackward();
    TEST_ASSERT_EQ(tower.targetingMode(), btd4::TargetingMode::Last);
    tower.cycleTargetingModeBackward();
    TEST_ASSERT_EQ(tower.targetingMode(), btd4::TargetingMode::First);
}


TEST_CASE(PlayerStateInitializesAndResets) {
    btd4::Player player(1, "Co-op Player");
    TEST_ASSERT_EQ(player.id(), static_cast<uint8_t>(1));
    TEST_ASSERT_EQ(player.name(), "Co-op Player");
    TEST_ASSERT(player.active());
    player.economy().spendCash(100);
    TEST_ASSERT_EQ(player.economy().cash(), 550);
    player.reset(900, 50);
    TEST_ASSERT_EQ(player.economy().cash(), 900);
    TEST_ASSERT_EQ(player.economy().lives(), 50);
    player.setActive(false);
    TEST_ASSERT(!player.active());
}


TEST_CASE(MultiplayerSessionConfiguresLocalPlayers) {
    btd4::MultiplayerSession session;
    std::string error;
    TEST_ASSERT(session.configure(3, btd4::MultiplayerEconomyMode::Split, error));
    TEST_ASSERT(error.empty());
    TEST_ASSERT_EQ(session.playerCount(), static_cast<uint8_t>(3));
    TEST_ASSERT_EQ(session.economyMode(), btd4::MultiplayerEconomyMode::Split);
    TEST_ASSERT(session.player(0).active());
    TEST_ASSERT(session.player(1).active());
    TEST_ASSERT(session.player(2).active());
    TEST_ASSERT(!session.player(3).active());
    TEST_ASSERT(session.start(error));
    TEST_ASSERT(session.active());
    session.stop();
    TEST_ASSERT(!session.active());
}


TEST_CASE(TowerOwnershipUsesPlayerEconomies) {
    btd4::GameSimulation simulation(btd4::Map("Ownership Test"));
    simulation.map().addPath(btd4::Path({{10.0f, 10.0f}, {100.0f, 10.0f}}));
    simulation.player(1).setActive(true);

    const int before0 = simulation.player(0).economy().cash();
    const int before1 = simulation.player(1).economy().cash();

    TEST_ASSERT(simulation.placeTower(static_cast<uint8_t>(1),
                                      btd4::TowerType::DartMonkey, 200.0f, 120.0f));
    TEST_ASSERT_EQ(simulation.towers().size(), size_t(1));
    TEST_ASSERT_EQ(simulation.towers()[0].ownerId(), static_cast<uint8_t>(1));
    TEST_ASSERT_EQ(simulation.player(0).economy().cash(), before0);
    TEST_ASSERT_EQ(simulation.player(1).economy().cash(), before1 - 200);

    TEST_ASSERT(!simulation.sellTower(static_cast<uint8_t>(0), simulation.towers()[0].id()));
    TEST_ASSERT(simulation.sellTower(static_cast<uint8_t>(1), simulation.towers()[0].id()));
    TEST_ASSERT_EQ(simulation.player(1).economy().cash(), before1 - 50);
}


TEST_CASE(MultiplayerControllerAssignments) {
    btd4::MultiplayerSession session;
    std::string error;
    TEST_ASSERT(session.configure(2, btd4::MultiplayerEconomyMode::Split, error));
    TEST_ASSERT(session.assignController(0, 7, error));
    TEST_ASSERT_EQ(session.playerForController(7), 0);
    TEST_ASSERT(!session.assignController(1, 7, error));
    TEST_ASSERT_EQ(session.playerForController(7), 0);
    TEST_ASSERT(session.assignController(1, 8, error));
    session.unassignController(7);
    TEST_ASSERT_EQ(session.playerForController(7), -1);
}


TEST_CASE(SharedEconomyModeUsesPlayerZeroPool) {
    btd4::GameSimulation simulation(btd4::Map("Shared Economy"));
    simulation.map().addPath(btd4::Path({{10.0f, 10.0f}, {100.0f, 10.0f}}));
    simulation.player(1).setActive(true);
    simulation.setEconomyMode(btd4::MultiplayerEconomyMode::Shared);

    const int before = simulation.player(0).economy().cash();
    TEST_ASSERT(simulation.placeTower(static_cast<uint8_t>(1),
                                      btd4::TowerType::DartMonkey, 200.0f, 120.0f));
    TEST_ASSERT_EQ(simulation.player(0).economy().cash(), before - 200);
    TEST_ASSERT_EQ(simulation.player(1).economy().cash(), before - 200);
}


TEST_CASE(ActivePlayerSelection) {
    btd4::GameSimulation simulation;
    simulation.player(1).setActive(true);
    TEST_ASSERT_EQ(simulation.activePlayerId(), static_cast<uint8_t>(0));
    TEST_ASSERT(simulation.setActivePlayer(1));
    TEST_ASSERT_EQ(simulation.activePlayerId(), static_cast<uint8_t>(1));
    TEST_ASSERT(!simulation.setActivePlayer(3));
    TEST_ASSERT_EQ(simulation.activePlayerId(), static_cast<uint8_t>(1));
}


TEST_CASE(ProjectileExplosionDoesNotRehitSpawnedChildren) {
    Map map("projectile-child-test");
    Path path;
    path.addWaypoint(0.0f, 0.0f);
    path.addWaypoint(100.0f, 0.0f);
    map.addPath(path);

    BloonPool bloons;
    ProjectilePool projectiles;
    Bloon* parent = bloons.spawn(BloonType::Rainbow, 0, 0.0f);
    TEST_ASSERT(parent != nullptr);
    parent->x = 20.0f;
    parent->y = 20.0f;

    Projectile* bomb = projectiles.spawn(
        ProjectileType::Bomb, DamageType::Explosive,
        20.0f, 20.0f, 1.0f, 0.0f,
        0.0f, 1, 1, 40.0f, 1.0f);
    TEST_ASSERT(bomb != nullptr);

    const int cash = projectiles.update(0.016f, bloons, map);
    TEST_ASSERT_EQ(cash, 1);
    TEST_ASSERT_EQ(bloons.activeCount(), static_cast<size_t>(4));
}
