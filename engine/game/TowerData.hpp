#pragma once

#include "Tower.hpp"
#include "core/IFileSystem.hpp"
#include <string>
#include <vector>

namespace btd4 {

struct TowerDefinition {
    std::string id;
    std::string displayName;
    TowerType type{TowerType::DartMonkey};
    TowerBaseStats stats;
};

struct TowerSet {
    std::vector<TowerDefinition> towers;
};

bool validateTowers(const TowerSet& towers, std::string& error);
bool parseTowers(const std::string& json, TowerSet& output, std::string& error);
bool loadTowers(const IFileSystem& files, const std::string& path,
                TowerSet& output, std::string& error);
std::string serializeTowers(const TowerSet& towers);
bool saveTowers(const std::string& path, const TowerSet& towers, std::string& error);

void configureTowerDefinitions(const TowerSet& towers);
const TowerSet& configuredTowerDefinitions();

const char* towerTypeName(TowerType type);
const char* projectileTypeName(ProjectileType type);
const char* damageTypeName(DamageType type);
bool parseTowerType(const std::string& value, TowerType& type);
bool parseProjectileType(const std::string& value, ProjectileType& type);
bool parseDamageType(const std::string& value, DamageType& type);

} // namespace btd4
