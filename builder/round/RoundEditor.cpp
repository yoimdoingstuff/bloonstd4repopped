#include "RoundEditor.hpp"

#include "../../engine/map/MapLoader.hpp"
#include "../../platform/common/NativeFileSystem.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <utility>

namespace btd4 {
namespace fs = std::filesystem;

RoundEditor::RoundEditor() = default;

void RoundEditor::initialize() {
    m_rounds.rounds.clear();
    m_selectedRound = 0;
    m_selectedGroup = 0;
    m_selectedFile = -1;
    m_status = "Round editor ready.";
    m_statusGood = true;
}

const char* RoundEditor::bloonName(BloonType type) {
    switch (type) {
        case BloonType::Red: return "Red";
        case BloonType::Blue: return "Blue";
        case BloonType::Green: return "Green";
        case BloonType::Yellow: return "Yellow";
        case BloonType::Pink: return "Pink";
        case BloonType::Black: return "Black";
        case BloonType::White: return "White";
        case BloonType::Lead: return "Lead";
        case BloonType::Rainbow: return "Rainbow";
        case BloonType::Ceramic: return "Ceramic";
        case BloonType::MOAB: return "MOAB";
        default: return "Unknown";
    }
}

BloonType RoundEditor::bloonTypeFromIndex(int index) {
    const int value = index + 1;
    if (value < static_cast<int>(BloonType::Red) || value > static_cast<int>(BloonType::MOAB)) {
        return BloonType::Red;
    }
    return static_cast<BloonType>(value);
}

void RoundEditor::normalizeSelection() {
    if (m_rounds.rounds.empty()) {
        m_selectedRound = 0;
        m_selectedGroup = 0;
        return;
    }
    m_selectedRound = std::clamp(m_selectedRound, 0, static_cast<int>(m_rounds.rounds.size()) - 1);
    auto& groups = m_rounds.rounds[m_selectedRound].groups;
    if (groups.empty()) {
        m_selectedGroup = 0;
    } else {
        m_selectedGroup = std::clamp(m_selectedGroup, 0, static_cast<int>(groups.size()) - 1);
    }
}

void RoundEditor::ensureDefaultLoaded(const std::string& projectRoot, const Map& map) {
    if (!m_rounds.rounds.empty() || !map.validate()) return;

    const fs::path defaultFile = fs::path(projectRoot) / "assets" / "placeholder" /
                                 "rounds" / "default_rounds.json";
    if (fs::exists(defaultFile)) {
        NativeFileSystem files;
        std::string error;
        RoundSet loaded;
        if (loadRounds(files, defaultFile.string(), map, loaded, error)) {
            m_rounds = std::move(loaded);
            m_status = "Loaded bundled round template.";
            m_statusGood = true;
        }
    }
    normalizeSelection();
}

void RoundEditor::refreshFiles(const std::string& projectRoot) {
    m_roundFiles.clear();
    const fs::path directory = fs::path(projectRoot) / "rounds";
    std::error_code ec;
    if (!fs::is_directory(directory, ec)) return;

    for (fs::directory_iterator it(directory, ec), end; it != end && !ec; it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == ".json") {
            m_roundFiles.push_back(it->path().string());
        }
    }
    std::sort(m_roundFiles.begin(), m_roundFiles.end());
    if (m_selectedFile >= static_cast<int>(m_roundFiles.size())) {
        m_selectedFile = m_roundFiles.empty() ? -1 : 0;
    }
}

void RoundEditor::newRound() {
    RoundDefinition round;
    round.groups.push_back(BloonGroup{});
    m_rounds.rounds.push_back(round);
    m_selectedRound = static_cast<int>(m_rounds.rounds.size()) - 1;
    m_selectedGroup = 0;
}

bool RoundEditor::loadFile(const std::string& path, const Map& map) {
    NativeFileSystem files;
    RoundSet loaded;
    std::string error;
    if (!loadRounds(files, path, map, loaded, error)) {
        m_status = "Load failed: " + error;
        m_statusGood = false;
        return false;
    }
    m_rounds = std::move(loaded);
    m_selectedRound = 0;
    m_selectedGroup = 0;
    normalizeSelection();
    m_status = "Loaded " + path;
    m_statusGood = true;
    return true;
}

bool RoundEditor::saveFile(const std::string& projectRoot, const Map& map) {
    std::string error;
    if (!validateRounds(m_rounds, map, error)) {
        m_status = "Validation failed: " + error;
        m_statusGood = false;
        return false;
    }

    const fs::path directory = fs::path(projectRoot) / "rounds";
    std::error_code ec;
    fs::create_directories(directory, ec);
    if (ec) {
        m_status = "Could not create rounds directory: " + ec.message();
        m_statusGood = false;
        return false;
    }

    const fs::path target = directory / "custom_rounds.json";
    if (!saveRounds(target.string(), m_rounds, map, error)) {
        m_status = "Save failed: " + error;
        m_statusGood = false;
        return false;
    }

    refreshFiles(projectRoot);
    m_status = "Saved " + target.string();
    m_statusGood = true;
    return true;
}

void RoundEditor::render(const std::string& projectRoot, const Map& map) {
    if (!ImGui::CollapsingHeader("Round Editor", ImGuiTreeNodeFlags_DefaultOpen)) return;

    if (!map.validate()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f),
                           "Create a valid map first. Round path indexes depend on the map.");
        return;
    }

    ensureDefaultLoaded(projectRoot, map);
    refreshFiles(projectRoot);
    normalizeSelection();

    if (ImGui::Button("New Round")) newRound();
    ImGui::SameLine();
    if (ImGui::Button("Add Group") && !m_rounds.rounds.empty()) {
        m_rounds.rounds[m_selectedRound].groups.push_back(BloonGroup{});
        m_selectedGroup = static_cast<int>(m_rounds.rounds[m_selectedRound].groups.size()) - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Round") && !m_rounds.rounds.empty()) {
        m_rounds.rounds.erase(m_rounds.rounds.begin() + m_selectedRound);
        normalizeSelection();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save Rounds")) saveFile(projectRoot, map);
    ImGui::SameLine();
    if (ImGui::Button("Validate Rounds")) {
        std::string error;
        if (validateRounds(m_rounds, map, error)) {
            m_status = "Round set is valid.";
            m_statusGood = true;
        } else {
            m_status = error;
            m_statusGood = false;
        }
    }

    if (!m_roundFiles.empty()) {
        std::vector<const char*> labels;
        labels.reserve(m_roundFiles.size());
        for (const auto& file : m_roundFiles) labels.push_back(file.c_str());
        if (m_selectedFile < 0) m_selectedFile = 0;
        if (ImGui::Combo("Saved Round Sets", &m_selectedFile,
                         labels.data(), static_cast<int>(labels.size()))) {
            if (m_selectedFile >= 0 &&
                m_selectedFile < static_cast<int>(m_roundFiles.size())) {
                loadFile(m_roundFiles[m_selectedFile], map);
            }
        }
    }

    if (m_rounds.rounds.empty()) {
        ImGui::TextDisabled("No rounds configured.");
        return;
    }

    std::vector<std::string> roundLabels;
    roundLabels.reserve(m_rounds.rounds.size());
    for (size_t i = 0; i < m_rounds.rounds.size(); ++i) {
        roundLabels.push_back("Round " + std::to_string(i + 1));
    }
    std::vector<const char*> roundLabelPtrs;
    roundLabelPtrs.reserve(roundLabels.size());
    for (const auto& label : roundLabels) roundLabelPtrs.push_back(label.c_str());
    ImGui::Combo("Round", &m_selectedRound, roundLabelPtrs.data(),
                 static_cast<int>(roundLabelPtrs.size()));

    auto& round = m_rounds.rounds[m_selectedRound];
    if (round.groups.empty()) round.groups.push_back(BloonGroup{});
    m_selectedGroup = std::clamp(m_selectedGroup, 0, static_cast<int>(round.groups.size()) - 1);

    std::vector<std::string> groupLabels;
    groupLabels.reserve(round.groups.size());
    for (size_t i = 0; i < round.groups.size(); ++i) {
        groupLabels.push_back("Group " + std::to_string(i + 1));
    }
    std::vector<const char*> groupLabelPtrs;
    groupLabelPtrs.reserve(groupLabels.size());
    for (const auto& label : groupLabels) groupLabelPtrs.push_back(label.c_str());
    ImGui::Combo("Group", &m_selectedGroup, groupLabelPtrs.data(),
                 static_cast<int>(groupLabelPtrs.size()));

    if (ImGui::Button("Delete Group") && round.groups.size() > 1) {
        round.groups.erase(round.groups.begin() + m_selectedGroup);
        m_selectedGroup = std::min(m_selectedGroup,
                                   static_cast<int>(round.groups.size()) - 1);
    }

    BloonGroup& group = round.groups[m_selectedGroup];
    int typeIndex = static_cast<int>(group.type) - 1;
    const char* typeNames[] = {"Red","Blue","Green","Yellow","Pink","Black",
                               "White","Lead","Rainbow","Ceramic","MOAB"};
    if (ImGui::Combo("Bloon Type", &typeIndex, typeNames, 11)) {
        group.type = bloonTypeFromIndex(typeIndex);
    }

    int count = static_cast<int>(group.count);
    int spacing = static_cast<int>(group.spacingMs);
    int delay = static_cast<int>(group.delayMs);
    int path = static_cast<int>(group.pathIndex);

    if (ImGui::InputInt("Count", &count)) group.count = static_cast<uint32_t>(std::clamp(count, 1, 100000));
    if (ImGui::InputInt("Spacing (ms)", &spacing)) group.spacingMs = static_cast<uint32_t>(std::clamp(spacing, 0, 3600000));
    if (ImGui::InputInt("Delay (ms)", &delay)) group.delayMs = static_cast<uint32_t>(std::clamp(delay, 0, 3600000));
    if (ImGui::InputInt("Path Index", &path)) group.pathIndex = static_cast<size_t>(std::clamp(path, 0, static_cast<int>(map.paths().size()) - 1));

    ImGui::Text("Group: %s x%d, every %d ms, after %d ms, path %d",
                bloonName(group.type), static_cast<int>(group.count),
                static_cast<int>(group.spacingMs), static_cast<int>(group.delayMs),
                static_cast<int>(group.pathIndex));

    if (!m_status.empty()) {
        ImGui::TextColored(m_statusGood ? ImVec4(0.3f,1.0f,0.4f,1.0f)
                                        : ImVec4(1.0f,0.4f,0.4f,1.0f),
                           "%s", m_status.c_str());
    }
}

} // namespace btd4
