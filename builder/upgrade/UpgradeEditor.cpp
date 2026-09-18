#include "UpgradeEditor.hpp"

#include "../../platform/common/NativeFileSystem.hpp"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <utility>

namespace btd4 {
namespace fs = std::filesystem;

void UpgradeEditor::initialize() {
    m_upgrades.upgrades.clear();
    m_selected = -1;
    m_selectedFile = -1;
    m_status = "Upgrade editor ready.";
    m_statusGood = true;
}

void UpgradeEditor::normalizeSelection() {
    if (m_upgrades.upgrades.empty()) {
        m_selected = -1;
        return;
    }
    m_selected = std::clamp(m_selected, 0, static_cast<int>(m_upgrades.upgrades.size()) - 1);
}

void UpgradeEditor::loadDefaults(const std::string& projectRoot) {
    if (!m_upgrades.upgrades.empty()) return;
    const fs::path path = fs::path(projectRoot) / "assets" / "placeholder" /
                          "upgrades" / "default_upgrades.json";
    if (!fs::exists(path)) return;
    NativeFileSystem files;
    std::string error;
    UpgradeSet loaded;
    if (loadUpgrades(files, path.string(), loaded, error)) {
        m_upgrades = std::move(loaded);
        m_selected = 0;
        m_status = "Loaded bundled upgrade template.";
        m_statusGood = true;
    } else {
        m_status = "Default upgrade load failed: " + error;
        m_statusGood = false;
    }
}

void UpgradeEditor::refreshFiles(const std::string& projectRoot) {
    m_files.clear();
    const fs::path dir = fs::path(projectRoot) / "upgrades";
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return;
    for (fs::directory_iterator it(dir, ec), end; it != end && !ec; it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == ".json")
            m_files.push_back(it->path().string());
    }
    std::sort(m_files.begin(), m_files.end());
    if (m_selectedFile >= static_cast<int>(m_files.size()))
        m_selectedFile = m_files.empty() ? -1 : 0;
}

bool UpgradeEditor::loadFile(const std::string& path) {
    NativeFileSystem files;
    UpgradeSet loaded;
    std::string error;
    if (!loadUpgrades(files, path, loaded, error)) {
        m_status = "Load failed: " + error;
        m_statusGood = false;
        return false;
    }
    m_upgrades = std::move(loaded);
    m_selected = 0;
    m_status = "Loaded " + path;
    m_statusGood = true;
    return true;
}

bool UpgradeEditor::saveFile(const std::string& projectRoot) {
    std::string error;
    if (!validateUpgrades(m_upgrades, error)) {
        m_status = "Validation failed: " + error;
        m_statusGood = false;
        return false;
    }
    const fs::path dir = fs::path(projectRoot) / "upgrades";
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        m_status = "Could not create upgrades directory: " + ec.message();
        m_statusGood = false;
        return false;
    }
    const fs::path target = dir / "custom_upgrades.json";
    if (!saveUpgrades(target.string(), m_upgrades, error)) {
        m_status = "Save failed: " + error;
        m_statusGood = false;
        return false;
    }
    refreshFiles(projectRoot);
    m_status = "Saved " + target.string();
    m_statusGood = true;
    return true;
}

void UpgradeEditor::render(const std::string& projectRoot) {
    if (!ImGui::CollapsingHeader("Upgrade Editor", ImGuiTreeNodeFlags_DefaultOpen)) return;

    loadDefaults(projectRoot);
    refreshFiles(projectRoot);
    normalizeSelection();

    if (ImGui::Button("Save Upgrades")) saveFile(projectRoot);
    ImGui::SameLine();
    if (ImGui::Button("Validate Upgrades")) {
        std::string error;
        if (validateUpgrades(m_upgrades, error)) {
            m_status = "Upgrade set is valid.";
            m_statusGood = true;
        } else {
            m_status = error;
            m_statusGood = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Defaults")) {
        m_upgrades.upgrades.clear();
        loadDefaults(projectRoot);
    }

    if (!m_files.empty()) {
        std::vector<const char*> labels;
        for (const auto& path : m_files) labels.push_back(path.c_str());
        if (m_selectedFile < 0) m_selectedFile = 0;
        if (ImGui::Combo("Saved Upgrade Sets", &m_selectedFile, labels.data(), static_cast<int>(labels.size()))) {
            if (m_selectedFile >= 0 && m_selectedFile < static_cast<int>(m_files.size()))
                loadFile(m_files[m_selectedFile]);
        }
    }

    if (m_upgrades.upgrades.empty()) {
        ImGui::TextDisabled("No upgrade definitions loaded.");
        return;
    }

    std::vector<std::string> names;
    std::vector<const char*> labels;
    for (const auto& upgrade : m_upgrades.upgrades) {
        names.push_back(upgrade.displayName + " [" + towerTypeName(upgrade.tower) + " " +
                        std::to_string(upgrade.path + 1) + "-" + std::to_string(upgrade.tier) + "]");
    }
    for (const auto& name : names) labels.push_back(name.c_str());
    ImGui::Combo("Upgrade", &m_selected, labels.data(), static_cast<int>(labels.size()));
    normalizeSelection();

    auto& upgrade = m_upgrades.upgrades[m_selected];
    auto& effect = upgrade.effect;
    char displayName[128];
    std::snprintf(displayName, sizeof(displayName), "%s", upgrade.displayName.c_str());
    if (ImGui::InputText("Display Name", displayName, sizeof(displayName)))
        upgrade.displayName = displayName;

    ImGui::Text("ID: %s", upgrade.id.c_str());

    const char* towers[] = {"DartMonkey","TackShooter","SniperMonkey","BoomerangThrower","BombTower","SuperMonkey"};
    int towerIndex = static_cast<int>(upgrade.tower);
    if (ImGui::Combo("Tower", &towerIndex, towers, 6))
        upgrade.tower = static_cast<TowerType>(std::clamp(towerIndex, 0, 5));

    int path = upgrade.path + 1;
    int tier = upgrade.tier;
    if (ImGui::InputInt("Path", &path)) upgrade.path = static_cast<uint8_t>(std::clamp(path - 1, 0, 1));
    if (ImGui::InputInt("Tier", &tier)) upgrade.tier = static_cast<uint8_t>(std::clamp(tier, 1, 4));

    int cost = effect.cost;
    int damage = effect.damageAdd;
    int pierce = effect.pierceAdd;
    if (ImGui::InputInt("Cost", &cost)) effect.cost = std::max(0, cost);
    if (ImGui::InputInt("Damage Add", &damage)) effect.damageAdd = damage;
    if (ImGui::InputInt("Pierce Add", &pierce)) effect.pierceAdd = pierce;
    if (ImGui::DragFloat("Range Add", &effect.rangeAdd, 0.5f, -1000.0f, 1000.0f)) {}
    if (ImGui::DragFloat("Cooldown Multiplier", &effect.cooldownMultiplier, 0.01f, 0.01f, 10.0f)) {}
    if (ImGui::DragFloat("Projectile Speed Multiplier", &effect.projectileSpeedMultiplier, 0.01f, 0.01f, 10.0f)) {}
    if (ImGui::DragFloat("Explosion Radius Add", &effect.explosionRadiusAdd, 0.5f, -1000.0f, 1000.0f)) {}

    if (!m_status.empty()) {
        ImGui::TextColored(m_statusGood ? ImVec4(0.3f,1.0f,0.4f,1.0f)
                                        : ImVec4(1.0f,0.4f,0.4f,1.0f),
                           "%s", m_status.c_str());
    }
}

} // namespace btd4
