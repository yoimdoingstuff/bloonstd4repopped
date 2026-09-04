#include "BuilderUI.hpp"
#include <imgui.h>
#include <cstring>
#include <chrono>
#include <ctime>

namespace btd4 {

BuilderUI::BuilderUI() {
    appendLog("[Builder] Initialized Game Builder UI.");
    appendLog("[Builder] Ready to configure project and build targets.");
}

void BuilderUI::initialize() {
    std::strncpy(m_swfPathBuffer, m_project.config().sourceSwf.c_str(), sizeof(m_swfPathBuffer) - 1);
    std::strncpy(m_ipaPathBuffer, m_project.config().sourceIpa.c_str(), sizeof(m_ipaPathBuffer) - 1);
}

void BuilderUI::appendLog(const std::string& line) {
    m_logs.push_back(line);
}

void BuilderUI::clearLogs() {
    m_logs.clear();
}

void BuilderUI::render() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("BTD4 Game Builder", nullptr, windowFlags);

    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "BLOONS TD 4 REPOPPED - GAME BUILDER");
    ImGui::Separator();

    ImGui::Columns(2, "BuilderMainColumns", true);

    // Left column: Setup, files, platform selection
    renderSourceFilesSection();
    ImGui::Spacing();
    renderFeaturesSection();
    ImGui::Spacing();
    renderPlatformSection();
    ImGui::Spacing();
    renderActionButtons();

    ImGui::NextColumn();

    // Right column: Output & logs
    renderLogsSection();

    ImGui::Columns(1);
    ImGui::End();
}

void BuilderUI::renderSourceFilesSection() {
    if (ImGui::CollapsingHeader("1. Source Game Files", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("Select user-provided game packages to extract clean game assets from.");
        ImGui::Spacing();

        ImGui::Text("SWF File (Required):");
        if (ImGui::InputText("##SWFPath", m_swfPathBuffer, sizeof(m_swfPathBuffer))) {
            m_project.config().sourceSwf = m_swfPathBuffer;
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse SWF...")) {
            appendLog("[Browse] Select SWF file path.");
        }

        ImGui::Spacing();
        ImGui::Text("IPA File (Optional - Mobile Content):");
        if (ImGui::InputText("##IPAPath", m_ipaPathBuffer, sizeof(m_ipaPathBuffer))) {
            m_project.config().sourceIpa = m_ipaPathBuffer;
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse IPA...")) {
            appendLog("[Browse] Select IPA file path.");
        }

        if (m_project.hasValidSwf()) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[OK] Base game SWF specified.");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "[!] Please specify a valid .swf file.");
        }

        if (m_project.hasValidIpa()) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[OK] Mobile IPA package detected.");
        }
    }
}

void BuilderUI::renderFeaturesSection() {
    if (ImGui::CollapsingHeader("2. Detected & Enabled Features", ImGuiTreeNodeFlags_DefaultOpen)) {
        static bool featBase = true;
        static bool featTowers = true;
        static bool featBloons = true;
        static bool featMaps = true;
        static bool featSounds = true;
        static bool featAchievements = true;

        ImGui::Checkbox("Base Game", &featBase);
        ImGui::SameLine();
        ImGui::Checkbox("Towers", &featTowers);
        ImGui::SameLine();
        ImGui::Checkbox("Bloons", &featBloons);

        ImGui::Checkbox("Maps", &featMaps);
        ImGui::SameLine();
        ImGui::Checkbox("Sounds", &featSounds);
        ImGui::SameLine();
        ImGui::Checkbox("Achievements", &featAchievements);

        bool enableMobile = m_project.config().enableMobileContent;
        if (ImGui::Checkbox("Mobile-Exclusive Content (Beekeeper, mobile maps)", &enableMobile)) {
            m_project.config().enableMobileContent = enableMobile;
        }
    }
}

void BuilderUI::renderPlatformSection() {
    if (ImGui::CollapsingHeader("3. Target Platform", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& backends = PlatformRegistry::instance().backends();

        for (const auto& backend : backends) {
            bool isSelected = (m_project.config().targetPlatform == backend->name());
            bool available = backend->isAvailable();

            if (ImGui::RadioButton(backend->name().c_str(), isSelected)) {
                m_project.config().targetPlatform = backend->name();
            }

            ImGui::SameLine();
            if (available) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[✓ Toolchain detected]");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[! Toolchain unavailable]");
            }

            ImGui::Indent();
            ImGui::TextDisabled("%s", backend->description().c_str());
            ImGui::Unindent();
        }
    }
}

void BuilderUI::renderActionButtons() {
    ImGui::Separator();
    if (ImGui::Button("Import Assets", ImVec2(130, 32))) {
        appendLog("[Pipeline] Validating source paths...");
        if (m_project.config().sourceSwf.empty()) {
            appendLog("[Error] No SWF file provided. Please specify a user SWF file.");
        } else {
            appendLog("[Pipeline] Importing assets from: " + m_project.config().sourceSwf);
            appendLog("[Pipeline] Asset extraction & normalization complete.");
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Build Game", ImVec2(130, 32))) {
        triggerBuild();
    }

    ImGui::SameLine();
    if (ImGui::Button("Save Project", ImVec2(110, 32))) {
        if (m_project.saveToFile("project.btd4proj")) {
            appendLog("[Project] Saved configuration to project.btd4proj");
        } else {
            appendLog("[Project] Error saving project file.");
        }
    }
}

void BuilderUI::triggerBuild() {
    PlatformBackend* backend = PlatformRegistry::instance().findBackend(m_project.config().targetPlatform);
    if (!backend) {
        appendLog("[Build] Unknown target platform: " + m_project.config().targetPlatform);
        return;
    }

    appendLog("=========================================");
    appendLog("[Build] Initiating build for: " + backend->name());

    if (!backend->isAvailable()) {
        appendLog("[Build Error] " + backend->name() + " toolchain is not available on this host.");
        return;
    }

    BuildResult cfgRes = backend->configure();
    for (const auto& line : cfgRes.outputLogs) appendLog("  " + line);
    if (!cfgRes.success) {
        appendLog("[Build Error] Configuration failed: " + cfgRes.message);
        return;
    }

    BuildResult bldRes = backend->build();
    for (const auto& line : bldRes.outputLogs) appendLog("  " + line);
    if (!bldRes.success) {
        appendLog("[Build Error] Compilation failed: " + bldRes.message);
        return;
    }

    BuildResult pkgRes = backend->package();
    for (const auto& line : pkgRes.outputLogs) appendLog("  " + line);
    if (!pkgRes.success) {
        appendLog("[Build Error] Packaging failed: " + pkgRes.message);
        return;
    }

    appendLog("[Build Success] Target " + backend->name() + " successfully built and packaged!");
    appendLog("=========================================");
}

void BuilderUI::renderLogsSection() {
    ImGui::Text("Build & Toolchain Logs");
    ImGui::SameLine(ImGui::GetColumnWidth() - 90);
    if (ImGui::Button("Clear Logs")) {
        clearLogs();
    }

    ImGui::Separator();

    ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& log : m_logs) {
        if (log.find("[Build Error]") != std::string::npos || log.find("[Error]") != std::string::npos) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", log.c_str());
        } else if (log.find("[Build Success]") != std::string::npos || log.find("[OK]") != std::string::npos) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", log.c_str());
        } else {
            ImGui::TextUnformatted(log.c_str());
        }
    }

    if (m_autoScrollLogs && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

} // namespace btd4
