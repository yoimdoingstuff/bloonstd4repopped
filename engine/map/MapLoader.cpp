#include "MapLoader.hpp"
#include "core/JsonReader.hpp"
#include <utility>

namespace btd4 {
namespace {
constexpr size_t MaxBytes = 1024 * 1024;
constexpr size_t MaxPaths = 64;
constexpr size_t MaxPoints = 4096;
constexpr size_t MaxRegions = 1024;
class Reader : public detail::JsonReader {
public:
    using JsonReader::JsonReader;
    Map read() {
        Map map;
        unsigned fields = 0;
        expect('{');
        do {
            const auto key = string();
            expect(':');
            unsigned bit = key == "version" ? 1 : key == "name" ? 2 :
                key == "paths" ? 4 : key == "buildable_regions" ? 8 :
                key == "blocked_regions" ? 16 : 0;
            if (!bit) fail("Unknown map field");
            if (fields & bit) fail("Duplicate map field");
            fields |= bit;
            if (bit == 1) {
                if (number() != 1) fail("Unsupported map version");
            } else if (bit == 2) {
                auto name = string();
                if (name.empty()) fail("Map name is empty");
                map.setName(name);
            } else if (bit == 4) {
                array(MaxPaths, [&] {
                    std::vector<Point2D> points;
                    array(MaxPoints, [&] {
                        expect('[');
                        const float x = coordinate(); expect(',');
                        const float y = coordinate(); expect(']');
                        points.push_back({x, y});
                    });
                    map.addPath(Path(std::move(points)));
                });
            } else {
                array(MaxRegions, [&] {
                    expect('[');
                    const float x = coordinate(); expect(',');
                    const float y = coordinate(); expect(',');
                    const float w = coordinate(); expect(',');
                    const float h = coordinate(); expect(']');
                    if (w <= 0 || h <= 0) fail("Region dimensions must be positive");
                    if (bit == 8) map.addBuildableRegion({x,y,w,h});
                    else map.addBlockedRegion({x,y,w,h});
                });
            }
        } while (take(','));
        expect('}');
        space();
        if (pos != source.size()) fail("Trailing map data");
        if ((fields & 7) != 7) fail("Required fields: version, name, paths");
        if (!map.validate()) fail("Map needs nonzero paths with at least two points");
        return map;
    }
private:
    float coordinate() {
        const double value = number();
        if (std::abs(value) > 1000000) fail("Coordinate limit exceeded");
        return static_cast<float>(value);
    }
};
}
bool parseMap(std::string_view json, Map& output, std::string& error) {
    error.clear();
    if (json.size() > MaxBytes) { error = "Map exceeds 1 MiB limit"; return false; }
    if (!detail::validUtf8(json)) { error = "Map is not valid UTF-8"; return false; }
    try {
        Map parsed = Reader(json).read();
        output = std::move(parsed);
        return true;
    } catch (const std::runtime_error& e) {
        error = e.what();
        return false;
    }
}
bool loadMap(const IFileSystem& files, const std::string& path, Map& output, std::string& error) {
    std::vector<uint8_t> bytes;
    if (!files.readFile(path, bytes)) { error = "Cannot read map: " + path; return false; }
    if (bytes.empty()) return parseMap({}, output, error);
    return parseMap(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()), output, error);
}
}
