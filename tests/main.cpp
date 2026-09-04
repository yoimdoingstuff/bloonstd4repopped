#include "TestRunner.hpp"
#include "../engine/core/Clock.hpp"
#include "../engine/core/Logger.hpp"
#include "../engine/rendering/LogicalResolution.hpp"
#include "../builder/project/Project.hpp"
#include "../platform/common/PlatformRegistry.hpp"
#include "../platform/common/NativeFileSystem.hpp"
#include "../platform/common/NullAudio.hpp"
#define SDL_MAIN_HANDLED
#include "../platform/common/SDLInput.hpp"
#include "../engine/map/Map.hpp"
#include "../engine/game/Bloon.hpp"
#include "../engine/game/Projectile.hpp"
#include "../engine/game/Tower.hpp"
#include "../engine/game/Economy.hpp"
#include "../engine/game/GameState.hpp"
#include <SDL.h>
#include <filesystem>
#include <thread>
#include <chrono>

// Test Clock functionality
TEST_CASE(ClockTickAndTimestep) {
    btd4::Clock clock(1.0 / 60.0);
    TEST_ASSERT_EQ(clock.frameCount(), 0);
    TEST_ASSERT(clock.deltaTime() >= 0.0);

    // Sleep for 15ms and tick
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    clock.tick();

    TEST_ASSERT_EQ(clock.frameCount(), 1);
    TEST_ASSERT(clock.deltaTime() > 0.0);
    TEST_ASSERT(clock.totalTime() > 0.0);
}

// Test Logical Resolution Viewport math
TEST_CASE(LogicalResolutionPillarbox) {
    // 960x544 is exactly 2x 480x272
    auto vpExact = btd4::LogicalResolution::calculateViewport(960, 544);
    TEST_ASSERT_EQ(vpExact.x, 0);
    TEST_ASSERT_EQ(vpExact.y, 0);
    TEST_ASSERT_EQ(vpExact.width, 960);
    TEST_ASSERT_EQ(vpExact.height, 544);

    // Ultrawide window: 1920x800 -> Pillarboxed (bars on sides)
    auto vpWide = btd4::LogicalResolution::calculateViewport(1920, 800);
    TEST_ASSERT(vpWide.x > 0);
    TEST_ASSERT_EQ(vpWide.y, 0);
    TEST_ASSERT_EQ(vpWide.height, 800);

    // Tall window: 800x1200 -> Letterboxed (bars on top and bottom)
    auto vpTall = btd4::LogicalResolution::calculateViewport(800, 1200);
    TEST_ASSERT_EQ(vpTall.x, 0);
    TEST_ASSERT(vpTall.y > 0);
    TEST_ASSERT_EQ(vpTall.width, 800);
}

// Test Screen-to-Logical coordinate mapping
TEST_CASE(LogicalResolutionCoordinateMapping) {
    btd4::Viewport vp{100, 50, 480, 272};
    float logX = 0.0f;
    float logY = 0.0f;

    // Click exactly at origin
    bool inBounds = btd4::LogicalResolution::screenToLogical(100, 50, vp, logX, logY);
    TEST_ASSERT(inBounds);
    TEST_ASSERT(std::abs(logX - 0.0f) < 0.001f);
    TEST_ASSERT(std::abs(logY - 0.0f) < 0.001f);

    // Click at center
    inBounds = btd4::LogicalResolution::screenToLogical(100 + 240, 50 + 136, vp, logX, logY);
    TEST_ASSERT(inBounds);
    TEST_ASSERT(std::abs(logX - 240.0f) < 0.001f);
    TEST_ASSERT(std::abs(logY - 136.0f) < 0.001f);

    // Reverse: Logical to screen
    int screenX = 0;
    int screenY = 0;
    btd4::LogicalResolution::logicalToScreen(240.0f, 136.0f, vp, screenX, screenY);
    TEST_ASSERT_EQ(screenX, 340);
    TEST_ASSERT_EQ(screenY, 186);
}

// Test Logger sinks and levels
TEST_CASE(LoggerCustomSink) {
    auto& logger = btd4::Logger::instance();
    std::string lastMessage;
    btd4::LogLevel lastLevel = btd4::LogLevel::Debug;

    logger.addSink([&](btd4::LogLevel level, const std::string& msg) {
        lastLevel = level;
        lastMessage = msg;
    });

    logger.info("Test info message");
    TEST_ASSERT_EQ(lastMessage, "Test info message");
    TEST_ASSERT(lastLevel == btd4::LogLevel::Info);

    logger.error("Test error message");
    TEST_ASSERT_EQ(lastMessage, "Test error message");
    TEST_ASSERT(lastLevel == btd4::LogLevel::Error);
}

// Test Project serialization and deserialization
TEST_CASE(ProjectSerialization) {
    btd4::Project project;
    project.config().projectName = "Custom BTD4";
    project.config().sourceSwf = "/path/to/game.swf";
    project.config().sourceIpa = "/path/to/game.ipa";
    project.config().enableMobileContent = true;
    project.config().targetPlatform = "PSP";
    project.config().buildConfiguration = "Release";

    TEST_ASSERT(project.hasValidSwf());
    TEST_ASSERT(project.hasValidIpa());

    std::string json = project.serialize();
    TEST_ASSERT(json.find("\"project_name\": \"Custom BTD4\"") != std::string::npos);
    TEST_ASSERT(json.find("\"target_platform\": \"PSP\"") != std::string::npos);

    btd4::Project loaded;
    bool ok = loaded.deserialize(json);
    TEST_ASSERT(ok);
    TEST_ASSERT_EQ(loaded.config().projectName, "Custom BTD4");
    TEST_ASSERT_EQ(loaded.config().sourceSwf, "/path/to/game.swf");
    TEST_ASSERT_EQ(loaded.config().sourceIpa, "/path/to/game.ipa");
    TEST_ASSERT_EQ(loaded.config().enableMobileContent, true);
    TEST_ASSERT_EQ(loaded.config().targetPlatform, "PSP");
}

// Test PlatformRegistry backends
TEST_CASE(PlatformRegistryBackends) {
    auto& reg = btd4::PlatformRegistry::instance();
    TEST_ASSERT(reg.backends().size() >= 4);

    btd4::PlatformBackend* linuxPlat = reg.findBackend("Linux");
    TEST_ASSERT(linuxPlat != nullptr);
    TEST_ASSERT_EQ(linuxPlat->name(), "Linux");

    btd4::PlatformBackend* pspPlat = reg.findBackend("PSP");
    TEST_ASSERT(pspPlat != nullptr);
    TEST_ASSERT_EQ(pspPlat->name(), "PSP");

    btd4::PlatformBackend* xboxPlat = reg.findBackend("Xbox 360");
    TEST_ASSERT(xboxPlat != nullptr);
    TEST_ASSERT_EQ(xboxPlat->name(), "Xbox 360");
}

TEST_CASE(PlatformBackendsDoNotReportUnimplementedBuildsAsSuccessful) {
    auto& registry = btd4::PlatformRegistry::instance();
    for (const auto& backend : registry.backends()) {
        const btd4::BuildResult configureResult = backend->configure();
        const btd4::BuildResult buildResult = backend->build();
        const btd4::BuildResult packageResult = backend->package();

        TEST_ASSERT(!configureResult.success);
        TEST_ASSERT(!configureResult.message.empty());
        TEST_ASSERT(!buildResult.success);
        TEST_ASSERT(!buildResult.message.empty());
        TEST_ASSERT(!packageResult.success);
        TEST_ASSERT(!packageResult.message.empty());
    }
}

TEST_CASE(NativeFileSystemReadWriteAndList) {
    namespace fs = std::filesystem;
    const fs::path testDirectory = fs::temp_directory_path() / "btd4_repopped_filesystem_test";
    fs::remove_all(testDirectory);

    btd4::NativeFileSystem fileSystem;
    const fs::path filePath = testDirectory / "nested" / "payload.bin";
    const std::vector<uint8_t> expected{0, 1, 2, 255};

    TEST_ASSERT(fileSystem.writeFile(filePath.u8string(), expected));
    TEST_ASSERT(fileSystem.fileExists(filePath.u8string()));

    std::vector<uint8_t> actual;
    TEST_ASSERT(fileSystem.readFile(filePath.u8string(), actual));
    TEST_ASSERT_EQ(actual, expected);

    std::vector<std::string> files;
    TEST_ASSERT(fileSystem.listFiles((testDirectory / "nested").u8string(), files));
    TEST_ASSERT_EQ(files.size(), static_cast<size_t>(1));
    TEST_ASSERT_EQ(files.front(), "payload.bin");
    TEST_ASSERT(fileSystem.removeFile(filePath.u8string()));
    TEST_ASSERT(!fileSystem.fileExists(filePath.u8string()));

    fs::remove_all(testDirectory);
}

TEST_CASE(NullAudioLifecycleAndVolumes) {
    btd4::NullAudio audio;
    TEST_ASSERT(!audio.isInitialized());
    TEST_ASSERT(audio.initialize());
    TEST_ASSERT(audio.isInitialized());

    audio.setMasterVolume(2.0f);
    audio.setMusicVolume(-1.0f);
    TEST_ASSERT_EQ(audio.masterVolume(), 1.0f);
    TEST_ASSERT_EQ(audio.musicVolume(), 0.0f);

    audio.playSound("tower_shot", 0.5f);
    audio.playMusic("menu_theme");
    audio.stopAllSounds();
    audio.stopMusic();
    audio.shutdown();
    TEST_ASSERT(!audio.isInitialized());
}

TEST_CASE(SDLInputTracksEdgesAndSeparateBindings) {
    btd4::SDLInput input;
    const btd4::Viewport viewport{0, 0, 480, 272};

    input.beginFrame();
    SDL_Event keyDown{};
    keyDown.type = SDL_KEYDOWN;
    keyDown.key.keysym.sym = SDLK_SPACE;
    input.processEvent(keyDown, viewport);
    TEST_ASSERT(input.isActionDown(btd4::InputAction::Confirm));
    TEST_ASSERT(input.isActionJustPressed(btd4::InputAction::Confirm));

    input.beginFrame();
    TEST_ASSERT(!input.isActionJustPressed(btd4::InputAction::Confirm));

    SDL_Event mouseDown{};
    mouseDown.type = SDL_MOUSEBUTTONDOWN;
    mouseDown.button.button = SDL_BUTTON_LEFT;
    mouseDown.button.x = 240;
    mouseDown.button.y = 136;
    input.processEvent(mouseDown, viewport);

    SDL_Event keyUp{};
    keyUp.type = SDL_KEYUP;
    keyUp.key.keysym.sym = SDLK_SPACE;
    input.processEvent(keyUp, viewport);
    TEST_ASSERT(input.isActionDown(btd4::InputAction::Confirm));

    input.beginFrame();
    SDL_Event mouseUp{};
    mouseUp.type = SDL_MOUSEBUTTONUP;
    mouseUp.button.button = SDL_BUTTON_LEFT;
    mouseUp.button.x = 240;
    mouseUp.button.y = 136;
    input.processEvent(mouseUp, viewport);
    TEST_ASSERT(input.isActionJustReleased(btd4::InputAction::Confirm));
    TEST_ASSERT(!input.isActionDown(btd4::InputAction::Confirm));

    input.beginFrame();
    keyDown.key.keysym.sym = SDLK_SPACE;
    keyDown.key.keysym.scancode = SDL_SCANCODE_SPACE;
    input.processEvent(keyDown, viewport);
    input.beginFrame();
    keyDown.key.keysym.sym = SDLK_RETURN;
    keyDown.key.keysym.scancode = SDL_SCANCODE_RETURN;
    input.processEvent(keyDown, viewport);
    keyUp.key.keysym.sym = SDLK_SPACE;
    keyUp.key.keysym.scancode = SDL_SCANCODE_SPACE;
    input.processEvent(keyUp, viewport);
    TEST_ASSERT(input.isActionDown(btd4::InputAction::Confirm));

    input.beginFrame();
    keyUp.key.keysym.sym = SDLK_RETURN;
    keyUp.key.keysym.scancode = SDL_SCANCODE_RETURN;
    input.processEvent(keyUp, viewport);
    TEST_ASSERT(input.isActionJustReleased(btd4::InputAction::Confirm));
    TEST_ASSERT(!input.isActionDown(btd4::InputAction::Confirm));
}

// -----------------------------------------------------------------------------
// Core Game & Economy Tests (Milestones 5 & 6)
// -----------------------------------------------------------------------------

TEST_CASE(MapPathInterpolation) {
    btd4::Path path;
    path.addWaypoint(0.0f, 0.0f);
    path.addWaypoint(100.0f, 0.0f);
    path.addWaypoint(100.0f, 100.0f);

    TEST_ASSERT_EQ(path.waypointCount(), static_cast<size_t>(3));
    TEST_ASSERT(std::abs(path.totalLength() - 200.0f) < 0.001f);

    // Start of path
    btd4::Point2D pStart = path.getPositionAtDistance(0.0f);
    TEST_ASSERT(std::abs(pStart.x - 0.0f) < 0.01f);
    TEST_ASSERT(std::abs(pStart.y - 0.0f) < 0.01f);
    TEST_ASSERT(!path.isAtEnd(0.0f));

    // Midpoint of first segment
    btd4::Point2D pMid1 = path.getPositionAtDistance(50.0f);
    TEST_ASSERT(std::abs(pMid1.x - 50.0f) < 0.01f);
    TEST_ASSERT(std::abs(pMid1.y - 0.0f) < 0.01f);

    // Corner point
    btd4::Point2D pCorner = path.getPositionAtDistance(100.0f);
    TEST_ASSERT(std::abs(pCorner.x - 100.0f) < 0.01f);
    TEST_ASSERT(std::abs(pCorner.y - 0.0f) < 0.01f);

    // Midpoint of second segment
    btd4::Point2D pMid2 = path.getPositionAtDistance(150.0f);
    TEST_ASSERT(std::abs(pMid2.x - 100.0f) < 0.01f);
    TEST_ASSERT(std::abs(pMid2.y - 50.0f) < 0.01f);

    // End of path
    TEST_ASSERT(path.isAtEnd(200.0f));
    TEST_ASSERT(path.isAtEnd(250.0f));

    // Map validation and placement
    btd4::Map map("Test Map");
    TEST_ASSERT(!map.validate()); // No paths yet

    map.addPath(path);
    TEST_ASSERT(map.validate());

    // Cannot place directly on path
    TEST_ASSERT(!map.canPlaceTower(50.0f, 0.0f, 12.0f));

    // Can place away from path
    TEST_ASSERT(map.canPlaceTower(50.0f, 50.0f, 12.0f));

    // Blocked region prevents placement
    map.addBlockedRegion({40.0f, 40.0f, 20.0f, 20.0f});
    TEST_ASSERT(!map.canPlaceTower(50.0f, 50.0f, 12.0f));
}

TEST_CASE(BloonPoppingAndChildHierarchy) {
    btd4::Bloon red;
    red.type = btd4::BloonType::Red;
    red.health = 1;
    red.active = true;

    std::vector<btd4::BloonType> children;
    bool damaged = red.takeDamage(1, btd4::DamageType::Sharp, children);
    TEST_ASSERT(damaged);
    TEST_ASSERT(red.popped);
    TEST_ASSERT_EQ(children.size(), static_cast<size_t>(0)); // Red has no children

    btd4::Bloon blue;
    blue.type = btd4::BloonType::Blue;
    blue.health = 1;
    blue.active = true;
    damaged = blue.takeDamage(1, btd4::DamageType::Sharp, children);
    TEST_ASSERT(damaged);
    TEST_ASSERT(blue.popped);
    TEST_ASSERT_EQ(children.size(), static_cast<size_t>(1));
    TEST_ASSERT(children[0] == btd4::BloonType::Red);

    // Immunity checks
    btd4::Bloon black;
    black.type = btd4::BloonType::Black;
    black.health = 1;
    black.active = true;
    damaged = black.takeDamage(1, btd4::DamageType::Explosive, children);
    TEST_ASSERT(!damaged); // Black immune to Explosive
    TEST_ASSERT(!black.popped);

    btd4::Bloon lead;
    lead.type = btd4::BloonType::Lead;
    lead.health = 1;
    lead.active = true;
    damaged = lead.takeDamage(1, btd4::DamageType::Sharp, children);
    TEST_ASSERT(!damaged); // Lead immune to Sharp
    TEST_ASSERT(!lead.popped);

    // Ceramic takes 10 damage to pop
    btd4::Bloon ceramic;
    ceramic.type = btd4::BloonType::Ceramic;
    ceramic.health = 10;
    ceramic.active = true;
    ceramic.takeDamage(4, btd4::DamageType::Sharp, children);
    TEST_ASSERT(!ceramic.popped);
    TEST_ASSERT_EQ(ceramic.health, 6);
    ceramic.takeDamage(6, btd4::DamageType::Sharp, children);
    TEST_ASSERT(ceramic.popped);
    TEST_ASSERT_EQ(children.size(), static_cast<size_t>(2));
    TEST_ASSERT(children[0] == btd4::BloonType::Rainbow);

    // RBE values
    TEST_ASSERT_EQ(btd4::getBloonRBE(btd4::BloonType::Red), 1);
    TEST_ASSERT_EQ(btd4::getBloonRBE(btd4::BloonType::Blue), 2);
    TEST_ASSERT_EQ(btd4::getBloonRBE(btd4::BloonType::Ceramic), 104);
    TEST_ASSERT_EQ(btd4::getBloonRBE(btd4::BloonType::MOAB), 616);
}

TEST_CASE(BloonPoolingAndSimulation) {
    btd4::BloonPool pool;
    TEST_ASSERT_EQ(pool.activeCount(), static_cast<size_t>(0));

    btd4::Bloon* b1 = pool.spawn(btd4::BloonType::Red, 0, 0.0f);
    TEST_ASSERT(b1 != nullptr);
    TEST_ASSERT_EQ(pool.activeCount(), static_cast<size_t>(1));
    TEST_ASSERT_EQ(b1->id, 1u);

    btd4::Bloon* b2 = pool.spawn(btd4::BloonType::Blue, 0, 10.0f);
    TEST_ASSERT(b2 != nullptr);
    TEST_ASSERT_EQ(pool.activeCount(), static_cast<size_t>(2));

    // Despawn b1 and verify slot recycling
    pool.despawn(b1->id);
    TEST_ASSERT_EQ(pool.activeCount(), static_cast<size_t>(1));

    btd4::Bloon* b3 = pool.spawn(btd4::BloonType::Green, 0, 0.0f);
    TEST_ASSERT(b3 != nullptr);
    TEST_ASSERT_EQ(pool.activeCount(), static_cast<size_t>(2));

    // Test movement along path
    btd4::Map map("Pool Map");
    btd4::Path path;
    path.addWaypoint(0.0f, 0.0f);
    path.addWaypoint(100.0f, 0.0f);
    map.addPath(path);

    std::vector<uint32_t> leaked;
    // Advance 1 second at 40px/sec
    pool.update(1.0f, map, leaked);
    TEST_ASSERT_EQ(leaked.size(), static_cast<size_t>(0));

    // Advance 5 seconds -> reaches end (100px)
    pool.update(5.0f, map, leaked);
    TEST_ASSERT(leaked.size() > 0);
}

TEST_CASE(TowerTargetingModes) {
    btd4::Tower tower(1, btd4::TowerType::DartMonkey, 50.0f, 50.0f);
    tower.setRange(100.0f);

    btd4::Bloon bFirst;
    bFirst.id = 1;
    bFirst.type = btd4::BloonType::Red;
    bFirst.x = 60.0f; bFirst.y = 50.0f;
    bFirst.distanceTraveled = 80.0f;
    bFirst.active = true;

    btd4::Bloon bLast;
    bLast.id = 2;
    bLast.type = btd4::BloonType::Red;
    bLast.x = 50.0f; bLast.y = 55.0f; // Very close (dist = 5)
    bLast.distanceTraveled = 20.0f;
    bLast.active = true;

    btd4::Bloon bStrong;
    bStrong.id = 3;
    bStrong.type = btd4::BloonType::Ceramic;
    bStrong.x = 80.0f; bStrong.y = 50.0f;
    bStrong.distanceTraveled = 50.0f;
    bStrong.active = true;

    std::vector<btd4::Bloon*> bloons = {&bFirst, &bLast, &bStrong};

    // First targeting mode
    tower.setTargetingMode(btd4::TargetingMode::First);
    const btd4::Bloon* target = tower.selectTarget(bloons);
    TEST_ASSERT(target != nullptr);
    TEST_ASSERT_EQ(target->id, bFirst.id);

    // Last targeting mode
    tower.setTargetingMode(btd4::TargetingMode::Last);
    target = tower.selectTarget(bloons);
    TEST_ASSERT(target != nullptr);
    TEST_ASSERT_EQ(target->id, bLast.id);

    // Close targeting mode
    tower.setTargetingMode(btd4::TargetingMode::Close);
    target = tower.selectTarget(bloons);
    TEST_ASSERT(target != nullptr);
    TEST_ASSERT_EQ(target->id, bLast.id);

    // Strong targeting mode
    tower.setTargetingMode(btd4::TargetingMode::Strong);
    target = tower.selectTarget(bloons);
    TEST_ASSERT(target != nullptr);
    TEST_ASSERT_EQ(target->id, bStrong.id);
}

TEST_CASE(ProjectileCollisionAndAoE) {
    btd4::Map map("Projectile Map");
    btd4::Path path;
    path.addWaypoint(0.0f, 0.0f);
    path.addWaypoint(200.0f, 0.0f);
    map.addPath(path);

    btd4::BloonPool bloonPool;
    btd4::ProjectilePool projectilePool;

    btd4::Bloon* bloon = bloonPool.spawn(btd4::BloonType::Red, 0, 50.0f);
    bloon->x = 50.0f;
    bloon->y = 50.0f;

    // Spawn a dart directly on top of the bloon
    btd4::Projectile* dart = projectilePool.spawn(
        btd4::ProjectileType::Dart,
        btd4::DamageType::Sharp,
        50.0f, 50.0f,
        1.0f, 0.0f,
        100.0f,
        1, 1
    );
    TEST_ASSERT(dart != nullptr);
    TEST_ASSERT_EQ(projectilePool.activeCount(), static_cast<size_t>(1));

    int cash = projectilePool.update(0.016f, bloonPool, map);
    TEST_ASSERT_EQ(cash, 1);
    TEST_ASSERT(bloon->popped);
    TEST_ASSERT_EQ(projectilePool.activeCount(), static_cast<size_t>(0)); // Dart depleted pierce

    // Test Area of Effect (Bomb)
    btd4::Bloon* b1 = bloonPool.spawn(btd4::BloonType::Red, 0, 10.0f);
    b1->x = 100.0f; b1->y = 100.0f;
    btd4::Bloon* b2 = bloonPool.spawn(btd4::BloonType::Red, 0, 15.0f);
    b2->x = 110.0f; b2->y = 100.0f;

    btd4::Projectile* bomb = projectilePool.spawn(
        btd4::ProjectileType::Bomb,
        btd4::DamageType::Explosive,
        100.0f, 100.0f,
        1.0f, 0.0f,
        100.0f,
        1, 1,
        30.0f // 30px explosion radius
    );
    TEST_ASSERT(bomb != nullptr);

    cash = projectilePool.update(0.016f, bloonPool, map);
    TEST_ASSERT_EQ(cash, 2); // Both bloons within 30px popped
    TEST_ASSERT(b1->popped);
    TEST_ASSERT(b2->popped);
}

TEST_CASE(EconomyTransactionsAndSellValues) {
    btd4::Economy eco(650, 100);
    TEST_ASSERT_EQ(eco.cash(), 650);
    TEST_ASSERT_EQ(eco.lives(), 100);

    TEST_ASSERT(eco.canAfford(200));
    TEST_ASSERT(!eco.canAfford(700));

    TEST_ASSERT(eco.spendCash(200));
    TEST_ASSERT_EQ(eco.cash(), 450);

    eco.addCash(150);
    TEST_ASSERT_EQ(eco.cash(), 600);

    eco.loseLives(30);
    TEST_ASSERT_EQ(eco.lives(), 70);
    TEST_ASSERT(!eco.isDefeated());

    eco.loseLives(70);
    TEST_ASSERT_EQ(eco.lives(), 0);
    TEST_ASSERT(eco.isDefeated());

    TEST_ASSERT_EQ(btd4::Economy::calculateRoundReward(1), 101);
    TEST_ASSERT_EQ(btd4::Economy::calculateRoundReward(20), 120);

    // Tower sell value test
    btd4::Tower tower(1, btd4::TowerType::DartMonkey, 10.0f, 10.0f);
    TEST_ASSERT_EQ(tower.totalInvestedCost(), 200);
    TEST_ASSERT_EQ(tower.sellValue(), 150); // 75% refund of 200 = 150
}

TEST_CASE(GameSimulationFullCycle) {
    btd4::Map map("Sim Map");
    btd4::Path path;
    path.addWaypoint(0.0f, 100.0f);
    path.addWaypoint(200.0f, 100.0f);
    map.addPath(path);

    btd4::GameSimulation sim(map);
    TEST_ASSERT(sim.state() == btd4::GameStateType::Playing);

    // Place a dart monkey next to track (cost 200, remaining cash 450)
    bool placed = sim.placeTower(btd4::TowerType::DartMonkey, 50.0f, 75.0f);
    TEST_ASSERT(placed);
    TEST_ASSERT_EQ(sim.towers().size(), static_cast<size_t>(1));
    TEST_ASSERT_EQ(sim.economy().cash(), 450);

    // Spawn a Red bloon at start of path
    btd4::Bloon* bloon = sim.bloonPool().spawn(btd4::BloonType::Red, 0, 0.0f);
    TEST_ASSERT(bloon != nullptr);

    // Step simulation until the tower shoots and pops the bloon
    for (int i = 0; i < 60; ++i) {
        sim.update(1.0f / 60.0f);
        if (sim.totalBloonsPopped() > 0) {
            break;
        }
    }

    TEST_ASSERT_EQ(sim.totalBloonsPopped(), 1);
    TEST_ASSERT_EQ(sim.economy().cash(), 451); // 450 + 1 cash from pop

    // Sell the tower (150 refund)
    bool sold = sim.sellTower(sim.towers()[0].id());
    TEST_ASSERT(sold);
    TEST_ASSERT_EQ(sim.towers().size(), static_cast<size_t>(0));
    TEST_ASSERT_EQ(sim.economy().cash(), 601);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return btd4::test::TestRunner::instance().run();
}

