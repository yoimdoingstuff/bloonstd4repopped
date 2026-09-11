#include "TestRunner.hpp"
#include "rendering/TextureAtlas.hpp"
#include "rendering/SpriteBatch.hpp"
#include "rendering/Camera.hpp"
#include "rendering/DisplayConfig.hpp"
#include "rendering/DebugRenderer.hpp"
#include <cmath>

namespace {

class MockRenderer : public btd4::IRenderer {
public:
    int drawSpriteCalls{0};
    int drawSpriteRegionCalls{0};
    int drawRectCalls{0};
    int drawLineCalls{0};
    int drawCircleCalls{0};
    int drawTextCalls{0};

    std::vector<std::string> drawnTextures;

    bool initialize(int, int) override { return true; }
    void shutdown() override {}
    void beginFrame() override {}
    void endFrame() override {}
    void setViewport(const btd4::Viewport&) override {}
    void clear(const btd4::Color&) override {}

    void drawRect(float, float, float, float, const btd4::Color&, bool) override { drawRectCalls++; }
    void drawLine(float, float, float, float, const btd4::Color&) override { drawLineCalls++; }
    void drawCircle(float, float, float, const btd4::Color&, bool) override { drawCircleCalls++; }
    void drawText(const std::string&, float, float, float, const btd4::Color&) override { drawTextCalls++; }

    bool loadTexture(const std::string&, const std::string&) override { return true; }
    bool hasTexture(const std::string&) const override { return true; }

    void drawSprite(const std::string& textureKey, float, float, float, float, float, const btd4::Color&) override {
        drawSpriteCalls++;
        drawnTextures.push_back(textureKey);
    }

    void drawSpriteRegion(const std::string& textureKey, const btd4::Rect&, float, float, float, float, float, const btd4::Color&) override {
        drawSpriteRegionCalls++;
        drawnTextures.push_back(textureKey);
    }

    void onResize(int, int) override {}
};

} // namespace

TEST_CASE(TextureAtlasRegionManagement) {
    btd4::TextureAtlas atlas;
    TEST_ASSERT_EQ(atlas.regionCount(), 0u);

    atlas.addRegion("dart_monkey", "monkeys_sheet", {0.0f, 0.0f, 32.0f, 32.0f}, 32, 32);
    atlas.addRegion("tack_shooter", "monkeys_sheet", {32.0f, 0.0f, 32.0f, 32.0f}, 32, 32);

    TEST_ASSERT_EQ(atlas.regionCount(), 2u);
    TEST_ASSERT(atlas.hasRegion("dart_monkey"));
    TEST_ASSERT(atlas.hasRegion("tack_shooter"));
    TEST_ASSERT(!atlas.hasRegion("super_monkey"));

    const auto* reg = atlas.getRegion("dart_monkey");
    TEST_ASSERT(reg != nullptr);
    TEST_ASSERT_EQ(reg->textureKey, "monkeys_sheet");
    TEST_ASSERT_EQ(reg->subRect.w, 32.0f);

    atlas.clear();
    TEST_ASSERT_EQ(atlas.regionCount(), 0u);
}

TEST_CASE(SpriteBatchGroupingAndSorting) {
    btd4::SpriteBatch batch;
    MockRenderer renderer;

    batch.begin();
    batch.draw("bloons", 10.0f, 10.0f, 16.0f, 16.0f, 0.0f, btd4::Color::white(), 2);
    batch.draw("towers", 50.0f, 50.0f, 32.0f, 32.0f, 0.0f, btd4::Color::white(), 1);
    batch.draw("projectiles", 20.0f, 20.0f, 8.0f, 8.0f, 0.0f, btd4::Color::white(), 3);
    batch.draw("background", 0.0f, 0.0f, 480.0f, 272.0f, 0.0f, btd4::Color::white(), 0);

    TEST_ASSERT_EQ(batch.getCommandCount(), 4u);

    batch.end(renderer);

    TEST_ASSERT_EQ(renderer.drawSpriteCalls, 4);
    TEST_ASSERT_EQ(renderer.drawnTextures.size(), 4u);
    // Verified sorted by zOrder: 0 (background) -> 1 (towers) -> 2 (bloons) -> 3 (projectiles)
    TEST_ASSERT_EQ(renderer.drawnTextures[0], "background");
    TEST_ASSERT_EQ(renderer.drawnTextures[1], "towers");
    TEST_ASSERT_EQ(renderer.drawnTextures[2], "bloons");
    TEST_ASSERT_EQ(renderer.drawnTextures[3], "projectiles");
}

TEST_CASE(CameraCoordinateTransformAndBounds) {
    btd4::Camera camera(480.0f, 272.0f);
    camera.setPosition(240.0f, 136.0f);
    camera.setZoom(1.0f);

    float sx = 0.0f;
    float sy = 0.0f;
    camera.worldToScreen(240.0f, 136.0f, sx, sy);
    TEST_ASSERT_EQ(sx, 240.0f);
    TEST_ASSERT_EQ(sy, 136.0f);

    float wx = 0.0f;
    float wy = 0.0f;
    camera.screenToWorld(240.0f, 136.0f, wx, wy);
    TEST_ASSERT_EQ(wx, 240.0f);
    TEST_ASSERT_EQ(wy, 136.0f);

    // Zooming 2x
    camera.setZoom(2.0f);
    camera.worldToScreen(250.0f, 136.0f, sx, sy);
    // (250 - 240) * 2 + 240 = 260
    TEST_ASSERT_EQ(sx, 260.0f);

    // Visible bounds check
    btd4::Rect visible = camera.getVisibleBounds();
    TEST_ASSERT_EQ(visible.w, 240.0f);
    TEST_ASSERT_EQ(visible.h, 136.0f);

    TEST_ASSERT(camera.isVisible({240.0f, 136.0f, 10.0f, 10.0f}));
    TEST_ASSERT(!camera.isVisible({1000.0f, 1000.0f, 10.0f, 10.0f}));
}

TEST_CASE(DisplayConfigProfilesAndCalculations) {
    auto pspRes = btd4::DisplayConfig::getProfileResolution(btd4::DisplayProfile::PSP_480x272);
    TEST_ASSERT_EQ(pspRes.width, 480);
    TEST_ASSERT_EQ(pspRes.height, 272);

    auto fhdRes = btd4::DisplayConfig::getProfileResolution(btd4::DisplayProfile::Desktop_1080p_16x9);
    TEST_ASSERT_EQ(fhdRes.width, 1920);
    TEST_ASSERT_EQ(fhdRes.height, 1080);

    // Integer scaling: 960x544 window with 480x272 base gives 2x scale
    btd4::Viewport vpInt = btd4::DisplayConfig::calculateViewport(960, 544, btd4::ScalingMode::IntegerOnly, {480, 272});
    TEST_ASSERT_EQ(vpInt.width, 960);
    TEST_ASSERT_EQ(vpInt.height, 544);
    TEST_ASSERT_EQ(vpInt.x, 0);
    TEST_ASSERT_EQ(vpInt.y, 0);

    // Stretch scaling fills window
    btd4::Viewport vpStretch = btd4::DisplayConfig::calculateViewport(1000, 500, btd4::ScalingMode::Stretch, {480, 272});
    TEST_ASSERT_EQ(vpStretch.width, 1000);
    TEST_ASSERT_EQ(vpStretch.height, 500);

    // Screen to logical coordinate conversion
    float lx = 0.0f;
    float ly = 0.0f;
    bool inside = btd4::DisplayConfig::screenToLogical(480, 272, vpInt, {480, 272}, lx, ly);
    TEST_ASSERT(inside);
    TEST_ASSERT_EQ(static_cast<int>(std::round(lx)), 240);
    TEST_ASSERT_EQ(static_cast<int>(std::round(ly)), 136);
}

TEST_CASE(DebugRendererDrawCalls) {
    MockRenderer renderer;

    btd4::DebugRenderer::drawTowerRange(renderer, 100.0f, 100.0f, 50.0f);
    TEST_ASSERT_EQ(renderer.drawCircleCalls, 2); // 1 filled circle + 1 outline circle

    btd4::DebugRenderer::drawBoundingBox(renderer, 10.0f, 10.0f, 20.0f, 20.0f);
    TEST_ASSERT_EQ(renderer.drawRectCalls, 1);

    btd4::Path path;
    path.addWaypoint(0.0f, 0.0f);
    path.addWaypoint(100.0f, 0.0f);
    path.addWaypoint(100.0f, 100.0f);
    btd4::DebugRenderer::drawPath(renderer, path);
    TEST_ASSERT_EQ(renderer.drawLineCalls, 2); // 2 connecting segments
    TEST_ASSERT_EQ(renderer.drawCircleCalls, 5); // 2 previous + 3 waypoints

    btd4::DebugRenderer::drawFps(renderer, 60.0);
    TEST_ASSERT_EQ(renderer.drawTextCalls, 1);
}
