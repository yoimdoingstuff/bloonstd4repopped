#include "TowerEditor.hpp"

#include "../../platform/common/NativeFileSystem.hpp"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <utility>

namespace btd4 {
namespace fs = std::filesystem;

void TowerEditor::initialize() {
    m_towers.towers.clear();
    m_selected = -1;
    m_selectedFile = -1;
    m_status = "Tower editor ready.";
    m_statusGood = true;
}

void TowerEditor::normalizeSelection() {
    if (m_towers.towers.empty()) {
        m_selected = -1;
        return;
    }
    m_selected = std::clamp(m_selected, 0, static_cast<int>(m_towers.towers.size()) - 1);
}

void TowerEditor::loadDefaults(const std::string& projectRoot) {
    if (!m_towers.towers.empty()) return;
    const fs::path path = fs::path(projectRoot) / "assets" / "placeholder" /
                          "towers" / "default_towers.json";
    if (!fs::exists(path)) return;
    NativeFileSystem files;
    std::string error;
    TowerSet loaded;
    if (loadTowers(files, path.string(), loaded, error)) {
        m_towers = std::move(loaded);
        m_selected = 0;
        m_status = "Loaded bundled tower template.";
        m_statusGood = true;
    } else {
        m_status = "Default tower load failed: " + error;
        m_statusGood = false;
    }
}

void TowerEditor::refreshFiles(const std::string& projectRoot) {
    m_files.clear();
    const fs::path dir = fs::path(projectRoot) / "towers";
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

bool TowerEditor::loadFile(const std::string& path) {
    NativeFileSystem files;
    TowerSet loaded;
    std::string error;
    if (!loadTowers(files, path, loaded, error)) {
        m_status = "Load failed: " + error;
        m_statusGood = false;
        return false;
    }
    m_towers = std::move(loaded);
    m_selected = 0;
    m_status = "Loaded " + path;
    m_statusGood = true;
    return true;
}

bool TowerEditor::saveFile(const std::string& projectRoot) {
    std::string error;
    if (!validateTowers(m_towers, error)) {
        m_status = "Validation failed: " + error;
        m_statusGood = false;
        return false;
    }
    const fs::path dir = fs::path(projectRoot) / "towers";
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        m_status = "Could not create towers directory: " + ec.message();
        m_statusGood = false;
        return false;
    }
    const fs::path target = dir / "custom_towers.json";
    if (!saveTowers(target.string(), m_towers, error)) {
        m_status = "Save failed: " + error;
        m_statusGood = false;
        return false;
    }
    refreshFiles(projectRoot);
    m_status = "Saved " + target.string();
    m_statusGood = true;
    return true;
}

void TowerEditor::render(const std::string& projectRoot) {
    if (!ImGui::CollapsingHeader("Tower Editor", ImGuiTreeNodeFlags_DefaultOpen)) return;

    loadDefaults(projectRoot);
    refreshFiles(projectRoot);
    normalizeSelection();

    if (ImGui::Button("Save Towers")) saveFile(projectRoot);
    ImGui::SameLine();
    if (ImGui::Button("Validate Towers")) {
        std::string error;
        if (validateTowers(m_towers, error)) {
            m_status = "Tower set is valid.";
            m_statusGood = true;
        } else {
            m_status = error;
            m_statusGood = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Defaults")) {
        m_towers.towers.clear();
        loadDefaults(projectRoot);
    }

    if (!m_files.empty()) {
        std::vector<const char*> labels;
        for (const auto& path : m_files) labels.push_back(path.c_str());
        if (m_selectedFile < 0) m_selectedFile = 0;
        if (ImGui::Combo("Saved Tower Sets", &m_selectedFile, labels.data(), static_cast<int>(labels.size()))) {
            if (m_selectedFile >= 0 && m_selectedFile < static_cast<int>(m_files.size()))
                loadFile(m_files[m_selectedFile]);
        }
    }

    if (m_towers.towers.empty()) {
        ImGui::TextDisabled("No tower definitions loaded.");
        return;
    }

    std::vector<std::string> names;
    std::vector<const char*> labels;
    for (const auto& tower : m_towers.towers) names.push_back(tower.displayName);
    for (const auto& name : names) labels.push_back(name.c_str());
    ImGui::Combo("Tower", &m_selected, labels.data(), static_cast<int>(labels.size()));
    normalizeSelection();

    auto& tower = m_towers.towers[m_selected];
    auto& stats = tower.stats;
    char displayName[128];
    std::snprintf(displayName, sizeof(displayName), "%s", tower.displayName.c_str());

    if (ImGui::InputText("Display Name", displayName, sizeof(displayName))) {
        tower.displayName = displayName;
    }
    ImGui::Text("ID: %s", tower.id.c_str());
    ImGui::Text("Type: %s", towerTypeName(tower.type));

    int cost = stats.cost;
    int damage = stats.projectileDamage;
    int pierce = stats.projectilePierce;
    if (ImGui::InputInt("Cost", &cost)) stats.cost = std::max(0, cost);
    if (ImGui::DragFloat("Range", &stats.range, 1.0f, 1.0f, 100000.0f)) {}
    if (ImGui::DragFloat("Attack Cooldown", &stats.attackCooldown, 0.01f, 0.001f, 60.0f)) {}
    if (ImGui::DragFloat("Footprint Radius", &stats.footprintRadius, 0.25f, 1.0f, 100.0f)) {}
    if (ImGui::InputInt("Projectile Damage", &damage)) stats.projectileDamage = std::max(0, damage);
    if (ImGui::InputInt("Projectile Pierce", &pierce)) stats.projectilePierce = std::max(0, pierce);
    if (ImGui::DragFloat("Projectile Speed", &stats.projectileSpeed, 1.0f, 0.0f, 100000.0f)) {}
    if (ImGui::DragFloat("Explosion Radius", &stats.explosionRadius, 1.0f, 0.0f, 1000.0f)) {}

    const char* projectiles[] = {"Dart","Tack","Bomb","Boomerang","SniperShot","Laser","Plasma"};
    int projectileIndex = static_cast<int>(stats.projectileType);
    if (ImGui::Combo("Projectile Type", &projectileIndex, projectiles, 7))
        stats.projectileType = static_cast<ProjectileType>(std::clamp(projectileIndex, 0, 6));

    const char* damages[] = {"Sharp","Explosive","Energy","All"};
    int damageIndex = static_cast<int>(stats.damageType);
    if (ImGui::Combo("Damage Type", &damageIndex, damages, 4))
        stats.damageType = static_cast<DamageType>(std::clamp(damageIndex, 0, 3));

    if (!m_status.empty()) {
        ImGui::TextColored(m_statusGood ? ImVec4(0.3f,1.0f,0.4f,1.0f)
                                        : ImVec4(1.0f,0.4f,0.4f,1.0f),
                           "%s", m_status.c_str());
    }
}

} // namespace btd4
