#include "MapLoader.hpp"
#include "core/JsonReader.hpp"
#include <utility>
#include <fstream>
#include <iomanip>
#include <sstream>

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
                key == "blocked_regions" ? 16 : key == "track_set" ? 32 :
                key == "bloon_density" ? 64 : 0;
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
            } else if (bit == 32) {
                const double value = number();
                if (value < 0 || value > 3 || std::floor(value) != value) fail("Invalid track set");
                map.setTrackSet(static_cast<uint8_t>(value));
            } else if (bit == 64) {
                const double value = number();
                if (value < 0 || value > 2 || std::floor(value) != value) fail("Invalid bloon density");
                map.setBloonDensity(static_cast<BloonDensity>(static_cast<uint8_t>(value)));
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


std::string btd4::serializeMap(const btd4::Map& map) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3);
    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"name\": \"" ;
    for (char ch : map.name()) {
        if (ch == '\\' || ch == '"') out << '\\';
        out << ch;
    }
    out << "\",\n";
    out << "  \"track_set\": " << static_cast<unsigned>(map.trackSet()) << ",\n";
    out << "  \"bloon_density\": " << static_cast<unsigned>(map.bloonDensity()) << ",\n";
    out << "  \"paths\": [\n";
    for (size_t p = 0; p < map.paths().size(); ++p) {
        const auto& points = map.paths()[p].waypoints();
        out << "    [";
        for (size_t i = 0; i < points.size(); ++i) {
            if (i) out << ", ";
            out << "[" << points[i].x << ", " << points[i].y << "]";
        }
        out << "]";
        if (p + 1 < map.paths().size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";
    out << "  \"buildable_regions\": [\n";
    for (size_t i = 0; i < map.buildableRegions().size(); ++i) {
        const auto& r = map.buildableRegions()[i];
        out << "    [" << r.x << ", " << r.y << ", " << r.w << ", " << r.h << "]";
        if (i + 1 < map.buildableRegions().size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";
    out << "  \"blocked_regions\": [\n";
    for (size_t i = 0; i < map.blockedRegions().size(); ++i) {
        const auto& r = map.blockedRegions()[i];
        out << "    [" << r.x << ", " << r.y << ", " << r.w << ", " << r.h << "]";
        if (i + 1 < map.blockedRegions().size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

bool btd4::saveMap(const std::string& path, const btd4::Map& map, std::string& error) {
    error.clear();
    if (!map.validate()) {
        error = "Map must contain at least one valid path with two or more waypoints.";
        return false;
    }
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        error = "Cannot write map: " + path;
        return false;
    }
    const std::string json = btd4::serializeMap(map);
    out.write(json.data(), static_cast<std::streamsize>(json.size()));
    if (!out.good()) {
        error = "Failed while writing map: " + path;
        return false;
    }
    return true;
}
