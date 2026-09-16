#pragma once

#include "Tower.hpp"
#include "core/IFileSystem.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace btd4 {

struct UpgradeEffect {
    int cost{0};
    float rangeAdd{0.0f};
    float cooldownMultiplier{1.0f};
    int damageAdd{0};
    int pierceAdd{0};
    float projectileSpeedMultiplier{1.0f};
    float explosionRadiusAdd{0.0f};
};

struct UpgradeDefinition {
    std::string id;
    TowerType tower{TowerType::DartMonkey};
    uint8_t path{0};
    uint8_t tier{0};
    std::string displayName;
    UpgradeEffect effect;
};

struct UpgradeSet {
    std::vector<UpgradeDefinition> upgrades;
};

bool validateUpgrades(const UpgradeSet& upgrades, std::string& error);
bool parseUpgrades(const std::string& json, UpgradeSet& output, std::string& error);
bool loadUpgrades(const IFileSystem& files, const std::string& path,
                  UpgradeSet& output, std::string& error);

const UpgradeDefinition* findUpgrade(const UpgradeSet& upgrades,
                                      TowerType tower, uint8_t path,
                                      uint8_t tier);

} // namespace btd4
