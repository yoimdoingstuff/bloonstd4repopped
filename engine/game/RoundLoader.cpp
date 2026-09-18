#include "Rounds.hpp"
#include "core/JsonReader.hpp"
#include <utility>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace btd4 {
namespace {
class Reader : public detail::JsonReader {
public:
    using JsonReader::JsonReader;
    RoundSet read() {
        RoundSet result;
        unsigned fields = 0;
        expect('{');
        do {
            auto key = string(); expect(':');
            unsigned bit = key == "version" ? 1 : key == "rounds" ? 2 : 0;
            field(fields, bit);
            if (bit == 1) { if (number() != 1) fail("Unsupported rounds version"); }
            else array(1024, [&] { result.rounds.push_back(round()); });
        } while (take(','));
        expect('}'); space();
        if (pos != source.size()) fail("Trailing rounds data");
        if (fields != 3) fail("Required fields: version, rounds");
        return result;
    }
private:
    void field(unsigned& fields, unsigned bit) {
        if (!bit) fail("Unknown rounds field");
        if (fields & bit) fail("Duplicate rounds field");
        fields |= bit;
    }
    uint32_t integer(uint32_t max) {
        double value = number();
        if (value < 0 || value > max || std::floor(value) != value) fail("Invalid integer");
        return static_cast<uint32_t>(value);
    }
    RoundDefinition round() {
        RoundDefinition result;
        expect('{');
        if (string() != "groups") fail("Expected groups field");
        expect(':');
        array(1024, [&] { result.groups.push_back(group()); });
        expect('}');
        return result;
    }
    BloonGroup group() {
        BloonGroup result;
        unsigned fields = 0;
        expect('{');
        do {
            auto key = string(); expect(':');
            unsigned bit = key == "type" ? 1 : key == "count" ? 2 :
                key == "spacing_ms" ? 4 : key == "delay_ms" ? 8 : key == "path" ? 16 : 0;
            field(fields, bit);
            if (bit == 1) {
                auto type = string();
                static constexpr const char* names[] = {
                    "red", "blue", "green", "yellow", "pink", "black", "white",
                    "lead", "rainbow", "ceramic", "moab"
                };
                result.type = BloonType::None;
                for (size_t i = 0; i < 11; ++i) {
                    if (type == names[i]) result.type = static_cast<BloonType>(i + 1);
                }
                if (result.type == BloonType::None) fail("Unsupported bloon type");
            } else if (bit == 2) result.count = integer(100000);
            else if (bit == 4) result.spacingMs = integer(3600000);
            else if (bit == 8) result.delayMs = integer(3600000);
            else result.pathIndex = integer(63);
        } while (take(','));
        expect('}');
        if ((fields & 7) != 7) fail("Required group fields: type, count, spacing_ms");
        return result;
    }
};
}
bool parseRounds(std::string_view json, const Map& map, RoundSet& output, std::string& error) {
    error.clear();
    if (json.size() > 1024 * 1024 || !detail::validUtf8(json)) {
        error = "Rounds exceed 1 MiB or contain invalid UTF-8"; return false;
    }
    try {
        RoundSet parsed = Reader(json).read();
        if (!validateRounds(parsed, map, error)) return false;
        output = std::move(parsed);
        return true;
    } catch (const std::runtime_error& e) {
        error = e.what(); return false;
    }
}
bool loadRounds(const IFileSystem& files, const std::string& path, const Map& map,
                RoundSet& output, std::string& error) {
    std::vector<uint8_t> bytes;
    if (!files.readFile(path, bytes)) { error = "Cannot read rounds: " + path; return false; }
    if (bytes.empty()) return parseRounds({}, map, output, error);
    return parseRounds(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()), map, output, error);
}
}


std::string btd4::serializeRounds(const btd4::RoundSet& rounds) {
    static const char* const names[] = {
        "none", "red", "blue", "green", "yellow", "pink",
        "black", "white", "lead", "rainbow", "ceramic", "moab"
    };

    std::ostringstream out;
    out << "{\n  \"version\": 1,\n  \"rounds\": [\n";
    for (size_t r = 0; r < rounds.rounds.size(); ++r) {
        out << "    {\n      \"groups\": [\n";
        const auto& groups = rounds.rounds[r].groups;
        for (size_t g = 0; g < groups.size(); ++g) {
            const auto& group = groups[g];
            out << "        {\"type\": \"" << names[static_cast<size_t>(group.type)]
                << "\", \"count\": " << group.count
                << ", \"spacing_ms\": " << group.spacingMs
                << ", \"delay_ms\": " << group.delayMs
                << ", \"path\": " << group.pathIndex << "}";
            if (g + 1 < groups.size()) out << ",";
            out << "\n";
        }
        out << "      ]\n    }";
        if (r + 1 < rounds.rounds.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
    return out.str();
}

bool btd4::saveRounds(const std::string& path, const btd4::RoundSet& rounds,
                      const btd4::Map& map, std::string& error) {
    error.clear();
    if (!btd4::validateRounds(rounds, map, error)) return false;
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        error = "Cannot write rounds: " + path;
        return false;
    }
    const std::string json = btd4::serializeRounds(rounds);
    out.write(json.data(), static_cast<std::streamsize>(json.size()));
    if (!out.good()) {
        error = "Failed while writing rounds: " + path;
        return false;
    }
    return true;
}
