#include "Upgrade.hpp"
#include "TowerData.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <limits>

namespace btd4 {
namespace {

std::string trim(std::string value) {
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char c) { return !std::isspace(c); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), value.end());
    return value;
}

bool number(const std::string& object, const char* key, double& out) {
    const std::string marker = std::string("\"") + key + "\"";
    const auto keyPos = object.find(marker);
    if (keyPos == std::string::npos) return false;
    const auto colon = object.find(':', keyPos + marker.size());
    if (colon == std::string::npos) return false;
    size_t end = colon + 1;
    while (end < object.size() && std::isspace(static_cast<unsigned char>(object[end]))) ++end;
    size_t consumed = 0;
    try {
        out = std::stod(object.substr(end), &consumed);
    } catch (...) {
        return false;
    }
    return consumed != 0;
}

bool stringValue(const std::string& object, const char* key, std::string& out) {
    const std::string marker = std::string("\"") + key + "\"";
    const auto keyPos = object.find(marker);
    if (keyPos == std::string::npos) return false;
    const auto colon = object.find(':', keyPos + marker.size());
    if (colon == std::string::npos) return false;
    const auto firstQuote = object.find('"', colon + 1);
    if (firstQuote == std::string::npos) return false;
    const auto secondQuote = object.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) return false;
    out = object.substr(firstQuote + 1, secondQuote - firstQuote - 1);
    return true;
}

bool towerType(const std::string& name, TowerType& type) {
    if (name == "DartMonkey") type = TowerType::DartMonkey;
    else if (name == "TackShooter") type = TowerType::TackShooter;
    else if (name == "SniperMonkey") type = TowerType::SniperMonkey;
    else if (name == "BoomerangThrower") type = TowerType::BoomerangThrower;
    else if (name == "BombTower") type = TowerType::BombTower;
    else if (name == "SuperMonkey") type = TowerType::SuperMonkey;
    else return false;
    return true;
}

} // namespace

bool validateUpgrades(const UpgradeSet& upgrades, std::string& error) {
    error.clear();
    if (upgrades.upgrades.empty()) {
        error = "Upgrade set must contain at least one upgrade";
        return false;
    }
    for (size_t i = 0; i < upgrades.upgrades.size(); ++i) {
        const auto& u = upgrades.upgrades[i];
        if (u.id.empty() || u.displayName.empty() || u.path > 1 || u.tier < 1 || u.tier > 4) {
            error = "Invalid upgrade identity, path or tier";
            return false;
        }
        if (u.effect.cost < 0 || !std::isfinite(u.effect.rangeAdd) ||
            !std::isfinite(u.effect.cooldownMultiplier) || u.effect.cooldownMultiplier <= 0.0f ||
            !std::isfinite(u.effect.projectileSpeedMultiplier) || u.effect.projectileSpeedMultiplier <= 0.0f ||
            !std::isfinite(u.effect.explosionRadiusAdd)) {
            error = "Invalid upgrade effect values";
            return false;
        }
        for (size_t j = i + 1; j < upgrades.upgrades.size(); ++j) {
            const auto& other = upgrades.upgrades[j];
            if (u.id == other.id || (u.tower == other.tower && u.path == other.path && u.tier == other.tier)) {
                error = "Duplicate upgrade id or tower/path/tier";
                return false;
            }
        }
    }
    return true;
}

bool parseUpgrades(const std::string& json, UpgradeSet& output, std::string& error) {
    UpgradeSet parsed;
    error.clear();

    const std::string marker = "\"upgrades\"";
    const auto keyPos = json.find(marker);
    if (keyPos == std::string::npos) {
        error = "Upgrade set is missing upgrades array";
        return false;
    }

    const auto arrayStart = json.find('[', keyPos + marker.size());
    if (arrayStart == std::string::npos) {
        error = "Upgrade set is missing upgrades array";
        return false;
    }

    bool quoted = false;
    int arrayDepth = 0;
    size_t arrayEnd = std::string::npos;
    for (size_t i = arrayStart; i < json.size(); ++i) {
        const char c = json[i];
        if (c == '"' && (i == 0 || json[i - 1] != '\\')) quoted = !quoted;
        if (quoted) continue;
        if (c == '[') {
            ++arrayDepth;
        } else if (c == ']') {
            --arrayDepth;
            if (arrayDepth == 0) {
                arrayEnd = i;
                break;
            }
        }
    }
    if (arrayEnd == std::string::npos) {
        error = "Unterminated upgrades array";
        return false;
    }

    const std::string array = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    size_t pos = 0;
    while ((pos = array.find('{', pos)) != std::string::npos) {
        size_t depth = 1;
        bool objectQuoted = false;
        size_t end = pos + 1;
        for (; end < array.size() && depth; ++end) {
            const char c = array[end];
            if (c == '"' && (end == 0 || array[end - 1] != '\\')) objectQuoted = !objectQuoted;
            if (!objectQuoted && c == '{') ++depth;
            if (!objectQuoted && c == '}') --depth;
        }
        if (depth != 0) {
            error = "Unterminated upgrade object";
            return false;
        }

        const std::string object = array.substr(pos, end - pos);
        std::string id, displayName, towerName;
        double path = 0, tier = 0, cost = 0, rangeAdd = 0, cooldown = 1,
               damage = 0, pierce = 0, speed = 1, explosion = 0;
        if (!stringValue(object, "id", id) || !stringValue(object, "tower", towerName)) {
            error = "Upgrade object is missing required fields";
            return false;
        }

        UpgradeDefinition u;
        u.id = trim(id);
        if (!stringValue(object, "displayName", displayName)) displayName = u.id;
        u.displayName = displayName;
        if (!towerType(towerName, u.tower) || !number(object, "path", path) ||
            !number(object, "tier", tier) || !number(object, "cost", cost)) {
            error = "Upgrade object is missing required fields";
            return false;
        }
        number(object, "rangeAdd", rangeAdd);
        number(object, "cooldownMultiplier", cooldown);
        number(object, "damageAdd", damage);
        number(object, "pierceAdd", pierce);
        number(object, "projectileSpeedMultiplier", speed);
        number(object, "explosionRadiusAdd", explosion);
        u.path = static_cast<uint8_t>(path);
        u.tier = static_cast<uint8_t>(tier);
        u.effect.cost = static_cast<int>(cost);
        u.effect.rangeAdd = static_cast<float>(rangeAdd);
        u.effect.cooldownMultiplier = static_cast<float>(cooldown);
        u.effect.damageAdd = static_cast<int>(damage);
        u.effect.pierceAdd = static_cast<int>(pierce);
        u.effect.projectileSpeedMultiplier = static_cast<float>(speed);
        u.effect.explosionRadiusAdd = static_cast<float>(explosion);
        parsed.upgrades.push_back(std::move(u));

        pos = end;
    }

    if (!validateUpgrades(parsed, error)) return false;
    output = std::move(parsed);
    return true;
}

bool loadUpgrades(const IFileSystem& files, const std::string& path,
                  UpgradeSet& output, std::string& error) {
    std::vector<uint8_t> bytes;
    if (!files.readFile(path, bytes)) {
        error = "Unable to read upgrade file: " + path;
        return false;
    }
    return parseUpgrades(std::string(bytes.begin(), bytes.end()), output, error);
}

const UpgradeDefinition* findUpgrade(const UpgradeSet& upgrades,
                                     TowerType tower, uint8_t path,
                                     uint8_t tier) {
    for (const auto& upgrade : upgrades.upgrades) {
        if (upgrade.tower == tower && upgrade.path == path && upgrade.tier == tier) return &upgrade;
    }
    return nullptr;
}

} // namespace btd4


std::string serializeUpgrades(const UpgradeSet& upgrades) {
    std::ostringstream out;
    out << "{\n  \"version\": 1,\n  \"upgrades\": [\n";
    for (size_t i = 0; i < upgrades.upgrades.size(); ++i) {
        const auto& u = upgrades.upgrades[i];
        const auto& e = u.effect;
        out << "    {\"id\": \"" << u.id
            << "\", \"tower\": \"" << towerTypeName(u.tower)
            << "\", \"path\": " << static_cast<int>(u.path)
            << ", \"tier\": " << static_cast<int>(u.tier)
            << ", \"displayName\": \"" << u.displayName
            << "\", \"cost\": " << e.cost
            << ", \"rangeAdd\": " << e.rangeAdd
            << ", \"cooldownMultiplier\": " << e.cooldownMultiplier
            << ", \"damageAdd\": " << e.damageAdd
            << ", \"pierceAdd\": " << e.pierceAdd
            << ", \"projectileSpeedMultiplier\": " << e.projectileSpeedMultiplier
            << ", \"explosionRadiusAdd\": " << e.explosionRadiusAdd << "}";
        if (i + 1 < upgrades.upgrades.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
    return out.str();
}

bool saveUpgrades(const std::string& path, const UpgradeSet& upgrades, std::string& error) {
    error.clear();
    if (!validateUpgrades(upgrades, error)) return false;
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        error = "Cannot write upgrades: " + path;
        return false;
    }
    const std::string json = serializeUpgrades(upgrades);
    out.write(json.data(), static_cast<std::streamsize>(json.size()));
    if (!out.good()) {
        error = "Failed while writing upgrades: " + path;
        return false;
    }
    return true;
}

} // namespace btd4
