#include "TestRunner.hpp"
#include "map/MapLoader.hpp"

namespace {
const std::string valid = R"({"version":1,"name":"Original test map","paths":[[[0,0],[100,0],[100,100]],[[0,20],[100,20]]],"buildable_regions":[[0,0,480,272]],"blocked_regions":[[200,200,20,20]]})";
struct MemoryFiles : btd4::IFileSystem {
    std::string content = valid;
    bool available = true;
    bool readFile(const std::string&, std::vector<uint8_t>& out) const override {
        if (!available) return false;
        out.assign(content.begin(), content.end()); return true;
    }
    bool writeFile(const std::string&, const std::vector<uint8_t>&) override { return false; }
    bool fileExists(const std::string&) const override { return available; }
    bool createDirectories(const std::string&) override { return false; }
    bool removeFile(const std::string&) override { return false; }
    bool listFiles(const std::string&, std::vector<std::string>&) const override { return false; }
};
}
TEST_CASE(MapLoaderLoadsGeometryAndPlacement) {
    btd4::Map map;
    std::string error = "stale";
    TEST_ASSERT(btd4::parseMap(valid, map, error));
    TEST_ASSERT(error.empty());
    TEST_ASSERT_EQ(map.name(), "Original test map");
    TEST_ASSERT_EQ(map.paths().size(), size_t(2));
    TEST_ASSERT_EQ(map.paths()[0].totalLength(), 200.0f);
    TEST_ASSERT_EQ(map.paths()[0].getPositionAtDistance(150).y, 50.0f);
    TEST_ASSERT(map.canPlaceTower(300, 100, 10));
    TEST_ASSERT(!map.canPlaceTower(210, 210, 10));
    TEST_ASSERT(!map.canPlaceTower(50, 0, 10));
    TEST_ASSERT(!map.canPlaceTower(500, 100, 10));
}
TEST_CASE(MapLoaderRejectsInvalidInputTransactionally) {
    const std::vector<std::string> invalid = {
        "", "{}", "[]", valid + " garbage", valid.substr(0, valid.size()-1),
        R"({"version":2,"name":"x","paths":[[[0,0],[1,1]]]})",
        R"({"version":1,"version":1,"name":"x","paths":[[[0,0],[1,1]]]})",
        R"({"version":1,"name":"x","paths":[]})",
        R"({"version":1,"name":"x","paths":[[[0,0]]]})",
        R"({"version":1,"name":"x","paths":[[[0,0],[0,0]]]})",
        R"({"version":1,"name":"x","paths":[[[0,0],[1e99,0]]]})",
        R"({"version":1,"name":"x","paths":[[[0,0],[01,0]]]})",
        R"({"version":1,"name":"x","paths":[[[0,0],[1.,0]]]})",
        R"({"version":1,"name":"x","paths":[[[0,0],[NaN,0]]]})",
        R"({"version":1,"name":"x","paths":[[[0,0],[1,0],]]})",
        R"({"version":1,"name":"\uD800","paths":[[[0,0],[1,0]]]})",
        R"({"version":1,"name":"\q","paths":[[[0,0],[1,0]]]})",
        R"({"version":1,"name":"x","paths":[[[0,0],[1,0]]],"blocked_regions":[[0,0,-1,2]]})",
        R"({"version":1,"name":"x","paths":[[[0,0],[1,0]]],"unknown":[]})",
        std::string(1024 * 1024 + 1, ' ')
    };
    btd4::Map map;
    std::string error;
    TEST_ASSERT(btd4::parseMap(valid, map, error));
    for (const auto& input : invalid) {
        TEST_ASSERT(!btd4::parseMap(input, map, error));
        TEST_ASSERT(!error.empty());
        TEST_ASSERT_EQ(map.name(), "Original test map");
        TEST_ASSERT_EQ(map.paths().size(), size_t(2));
    }
}
TEST_CASE(MapLoaderReadsAbstractFileSystemAndEscapes) {
    MemoryFiles files;
    btd4::Map map;
    std::string error;
    TEST_ASSERT(btd4::loadMap(files, "map.json", map, error));
    files.available = false;
    TEST_ASSERT(!btd4::loadMap(files, "missing.json", map, error));
    TEST_ASSERT_EQ(map.name(), "Original test map");
    TEST_ASSERT(!error.empty());
    TEST_ASSERT(btd4::parseMap(R"({"paths":[[[-1e2,0],[0,2.5]]],"name":"A\"B\u00e9\uD83D\uDE00","version":1})", map, error));
    TEST_ASSERT_EQ(map.name(), std::string("A\"B\xc3\xa9\xf0\x9f\x98\x80"));
}
TEST_CASE(MapLoaderBoundsCollections) {
    std::string source = R"({"version":1,"name":"x","paths":[)";
    for (int i = 0; i < 65; ++i) {
        if (i) source += ',';
        source += "[[0,0],[1,0]]";
    }
    source += "]}";
    btd4::Map map;
    std::string error;
    TEST_ASSERT(!btd4::parseMap(source, map, error));
}
TEST_CASE(MapLoaderRejectsInvalidUtf8AndOversizedStrings) {
    btd4::Map map;
    std::string error;
    for (const auto& name : {std::string("\xc0\xaf"), std::string("\xed\xa0\x80"), std::string(1025, 'x')}) {
        TEST_ASSERT(!btd4::parseMap(std::string(R"({"version":1,"name":")") + name + R"(","paths":[[[0,0],[1,1]]]})", map, error));
    }
}
