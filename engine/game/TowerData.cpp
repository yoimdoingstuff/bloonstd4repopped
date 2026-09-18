#include "TowerData.hpp"
#include "../core/JsonReader.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace btd4 {
namespace {

TowerSet g_configured;

class Reader : public detail::JsonReader {
public:
    using JsonReader::JsonReader;

    TowerSet read() {
        TowerSet result;
        unsigned fields = 0;
        expect('{');
        do {
            const auto key = string();
            expect(':');
            const unsigned bit = key == "version" ? 1 : key == "towers" ? 2 : 0;
            if (!bit || (fields & bit)) fail("Invalid or duplicate tower set field");
            fields |= bit;
            if (bit == 1) {
                if (number() != 1) fail("Unsupported tower version");
            } else {
                array(64, [&] { result.towers.push_back(tower()); });
            }
        } while (take(','));
        expect('}');
        space();
        if (pos != source.size()) fail("Trailing tower data");
        if (fields != 3) fail("Required fields: version, towers");
        return result;
    }

private:
    static float finiteFloat(double value, const char* label) {
        if (!std::isfinite(value) || value < 0.0 || value > 1000000.0) fail(std::string("Invalid ") + label);
        return static_cast<float>(value);
    }

    static int nonNegativeInt(double value, const char* label) {
        if (!std::isfinite(value) || value < 0.0 || value > 1000000.0 || std::floor(value) != value)
            fail(std::string("Invalid ") + label);
        return static_cast<int>(value);
    }

    TowerDefinition tower() {
        TowerDefinition result;
        unsigned fields = 0;
        expect('{');
        do {
            const auto key = string();
            expect(':');
            const unsigned bit =
                key == "id" ? 1 :
                key == "displayName" ? 2 :
                key == "type" ? 4 :
                key == "cost" ? 8 :
                key == "range" ? 16 :
                key == "attackCooldown" ? 32 :
                key == "footprintRadius" ? 64 :
                key == "projectileType" ? 128 :
                key == "damageType" ? 256 :
                key == "projectileDamage" ? 512 :
                key == "projectilePierce" ? 1024 :
                key == "projectileSpeed" ? 2048 :
                key == "explosionRadius" ? 4096 : 0;
            if (!bit || (fields & bit)) fail("Invalid or duplicate tower field");
            fields |= bit;

            if (bit == 1) result.id = string();
            else if (bit == 2) result.displayName = string();
            else if (bit == 4) {
                TowerType type;
                if (!parseTowerType(string(), type)) fail("Unsupported tower type");
                result.type = type;
            } else if (bit == 8) result.stats.cost = nonNegativeInt(number(), "cost");
            else if (bit == 16) result.stats.range = finiteFloat(number(), "range");
            else if (bit == 32) result.stats.attackCooldown = finiteFloat(number(), "attackCooldown");
            else if (bit == 64) result.stats.footprintRadius = finiteFloat(number(), "footprintRadius");
            else if (bit == 128) {
                if (!parseProjectileType(string(), result.stats.projectileType)) fail("Unsupported projectile type");
            } else if (bit == 256) {
                if (!parseDamageType(string(), result.stats.damageType)) fail("Unsupported damage type");
            } else if (bit == 512) result.stats.projectileDamage = nonNegativeInt(number(), "projectileDamage");
            else if (bit == 1024) result.stats.projectilePierce = nonNegativeInt(number(), "projectilePierce");
            else if (bit == 2048) result.stats.projectileSpeed = finiteFloat(number(), "projectileSpeed");
            else result.stats.explosionRadius = finiteFloat(number(), "explosionRadius");
        } while (take(','));
        expect('}');
        return result;
    }
};

} // namespace

const char* towerTypeName(TowerType type) {
    switch (type) {
        case TowerType::DartMonkey: return "DartMonkey";
        case TowerType::TackShooter: return "TackShooter";
        case TowerType::SniperMonkey: return "SniperMonkey";
        case TowerType::BoomerangThrower: return "BoomerangThrower";
        case TowerType::BombTower: return "BombTower";
        case TowerType::SuperMonkey: return "SuperMonkey";
        default: return "DartMonkey";
    }
}

const char* projectileTypeName(ProjectileType type) {
    switch (type) {
        case ProjectileType::Dart: return "Dart";
        case ProjectileType::Tack: return "Tack";
        case ProjectileType::Bomb: return "Bomb";
        case ProjectileType::Boomerang: return "Boomerang";
        case ProjectileType::SniperShot: return "SniperShot";
        case ProjectileType::Laser: return "Laser";
        case ProjectileType::Plasma: return "Plasma";
        default: return "Dart";
    }
}

const char* damageTypeName(DamageType type) {
    switch (type) {
        case DamageType::Sharp: return "Sharp";
        case DamageType::Explosive: return "Explosive";
        case DamageType::Energy: return "Energy";
        case DamageType::All: return "All";
        default: return "Sharp";
    }
}

bool parseTowerType(const std::string& value, TowerType& type) {
    for (int i = 0; i < 6; ++i) {
        const auto candidate = static_cast<TowerType>(i);
        if (value == towerTypeName(candidate)) {
            type = candidate;
            return true;
        }
    }
    return false;
}

bool parseProjectileType(const std::string& value, ProjectileType& type) {
    for (int i = 0; i < 7; ++i) {
        const auto candidate = static_cast<ProjectileType>(i);
        if (value == projectileTypeName(candidate)) {
            type = candidate;
            return true;
        }
    }
    return false;
}

bool parseDamageType(const std::string& value, DamageType& type) {
    for (int i = 0; i < 4; ++i) {
        const auto candidate = static_cast<DamageType>(i);
        if (value == damageTypeName(candidate)) {
            type = candidate;
            return true;
        }
    }
    return false;
}

bool validateTowers(const TowerSet& towers, std::string& error) {
    error.clear();
    if (towers.towers.empty() || towers.towers.size() > 64) {
        error = "Expected 1..64 tower definitions";
        return false;
    }
    for (size_t i = 0; i < towers.towers.size(); ++i) {
        const auto& tower = towers.towers[i];
        const auto& s = tower.stats;
        if (tower.id.empty() || tower.displayName.empty() ||
            s.cost < 0 || !std::isfinite(s.range) || s.range <= 0.0f ||
            !std::isfinite(s.attackCooldown) || s.attackCooldown <= 0.0f ||
            !std::isfinite(s.footprintRadius) || s.footprintRadius <= 0.0f ||
            s.projectileDamage < 0 || s.projectilePierce < 0 ||
            !std::isfinite(s.projectileSpeed) || s.projectileSpeed < 0.0f ||
            !std::isfinite(s.explosionRadius) || s.explosionRadius < 0.0f) {
            error = "Invalid tower stats";
            return false;
        }
        for (size_t j = i + 1; j < towers.towers.size(); ++j) {
            if (tower.id == towers.towers[j].id || tower.type == towers.towers[j].type) {
                error = "Duplicate tower id or type";
                return false;
            }
        }
    }
    return true;
}

bool parseTowers(const std::string& json, TowerSet& output, std::string& error) {
    error.clear();
    if (json.size() > 1024 * 1024 || !detail::validUtf8(json)) {
        error = "Tower data exceeds 1 MiB or contains invalid UTF-8";
        return false;
    }
    try {
        TowerSet parsed = Reader(json).read();
        if (!validateTowers(parsed, error)) return false;
        output = std::move(parsed);
        return true;
    } catch (const std::runtime_error& e) {
        error = e.what();
        return false;
    }
}

bool loadTowers(const IFileSystem& files, const std::string& path,
                TowerSet& output, std::string& error) {
    std::vector<uint8_t> bytes;
    if (!files.readFile(path, bytes)) {
        error = "Unable to read tower file: " + path;
        return false;
    }
    return parseTowers(std::string(bytes.begin(), bytes.end()), output, error);
}

std::string serializeTowers(const TowerSet& towers) {
    std::ostringstream out;
    out << "{\n  \"version\": 1,\n  \"towers\": [\n";
    for (size_t i = 0; i < towers.towers.size(); ++i) {
        const auto& t = towers.towers[i];
        const auto& s = t.stats;
        out << "    {\"id\": \"" << t.id
            << "\", \"displayName\": \"" << t.displayName
            << "\", \"type\": \"" << towerTypeName(t.type)
            << "\", \"cost\": " << s.cost
            << ", \"range\": " << s.range
            << ", \"attackCooldown\": " << s.attackCooldown
            << ", \"footprintRadius\": " << s.footprintRadius
            << ", \"projectileType\": \"" << projectileTypeName(s.projectileType)
            << "\", \"damageType\": \"" << damageTypeName(s.damageType)
            << "\", \"projectileDamage\": " << s.projectileDamage
            << ", \"projectilePierce\": " << s.projectilePierce
            << ", \"projectileSpeed\": " << s.projectileSpeed
            << ", \"explosionRadius\": " << s.explosionRadius << "}";
        if (i + 1 < towers.towers.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
    return out.str();
}

bool saveTowers(const std::string& path, const TowerSet& towers, std::string& error) {
    error.clear();
    if (!validateTowers(towers, error)) return false;
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        error = "Cannot write towers: " + path;
        return false;
    }
    const auto json = serializeTowers(towers);
    out.write(json.data(), static_cast<std::streamsize>(json.size()));
    if (!out.good()) {
        error = "Failed while writing towers: " + path;
        return false;
    }
    return true;
}

void configureTowerDefinitions(const TowerSet& towers) {
    g_configured = towers;
}

const TowerSet& configuredTowerDefinitions() {
    return g_configured;
}

} // namespace btd4
