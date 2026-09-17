#include "BuilderUI.hpp"
#include "../../tools/asset_importer/AssetImporter.hpp"
#include "../platform/FileDialog.hpp"
#include <imgui.h>
#include <cstring>
#include <filesystem>

namespace btd4 {

BuilderUI::BuilderUI() {
    appendLog("[Builder] Initialized Game Builder UI.");
    appendLog("[Builder] Ready to configure project and build targets.");
}

void BuilderUI::initialize() {
    std::strncpy(m_sourceDirectoryBuffer, m_project.config().sourceDirectory.c_str(), sizeof(m_sourceDirectoryBuffer) - 1);
    m_sourceDirectoryBuffer[sizeof(m_sourceDirectoryBuffer) - 1] = '\0';
    std::strncpy(m_swfPathBuffer, m_project.config().sourceSwf.c_str(), sizeof(m_swfPathBuffer) - 1);
    m_swfPathBuffer[sizeof(m_swfPathBuffer) - 1] = '\0';
    std::strncpy(m_ipaPathBuffer, m_project.config().sourceIpa.c_str(), sizeof(m_ipaPathBuffer) - 1);
    m_ipaPathBuffer[sizeof(m_ipaPathBuffer) - 1] = '\0';
    discoverAssets();
}

void BuilderUI::appendLog(const std::string& line) { m_logs.push_back(line); }
void BuilderUI::clearLogs() { m_logs.clear(); }

void BuilderUI::addSourceFile(const std::string& path) {
    if (m_project.addSourceFile(path)) {
        std::strncpy(m_swfPathBuffer, m_project.config().sourceSwf.c_str(), sizeof(m_swfPathBuffer) - 1);
        m_swfPathBuffer[sizeof(m_swfPathBuffer) - 1] = '\0';
        std::strncpy(m_ipaPathBuffer, m_project.config().sourceIpa.c_str(), sizeof(m_ipaPathBuffer) - 1);
        m_ipaPathBuffer[sizeof(m_ipaPathBuffer) - 1] = '\0';
        appendLog("[Assets] Added source file: " + path);
    } else appendLog("[Assets] Ignored unsupported or missing file: " + path);
}

void BuilderUI::render() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("BTD4 Game Builder", nullptr, windowFlags);
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "BLOONS TD 4 REPOPPED - GAME BUILDER");
    ImGui::Separator();
    if (ImGui::BeginTable("BuilderMainTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextColumn(); renderSourceFilesSection(); ImGui::Spacing(); renderFeaturesSection();
        ImGui::Spacing(); renderPlatformSection(); ImGui::Spacing(); renderActionButtons();
        ImGui::TableNextColumn(); renderLogsSection(); ImGui::EndTable();
    }
    ImGui::End();
}

void BuilderUI::renderSourceFilesSection() {
    if (ImGui::CollapsingHeader("1. Source Game Files", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("Select or drop your own source files. Nothing is copied into the repository.");
        ImGui::TextDisabled("Accepted: .SWF for Flash/Expansion, .IPA for HD/mobile content.");
        ImGui::Spacing();
        ImGui::Text("Game Edition:");
        const char* editions[] = { "BTD4 Flash", "BTD4 Expansion", "BTD4 HD (iPad)" };
        int editionIndex = 0;
        for (int i = 0; i < 3; ++i) if (m_project.config().gameEdition == editions[i]) editionIndex = i;
        if (ImGui::Combo("##GameEdition", &editionIndex, editions, 3)) {
            m_project.config().gameEdition = editions[editionIndex];
            if (editionIndex == 2) m_project.config().enableMobileContent = true;
            appendLog("[Project] Game edition: " + m_project.config().gameEdition);
        }
        if (editionIndex == 0) ImGui::TextDisabled("Original Flash release. Source-specific maps/content are kept separate.");
        if (editionIndex == 1) ImGui::TextDisabled("Expansion release. Exclusive expansion content is treated as its own source set.");
        if (editionIndex == 2) ImGui::TextDisabled("iPad HD release. Mobile-only maps/assets can be imported without replacing Flash content.");
        ImGui::Spacing();
        ImGui::Text("Asset Folder:");
        if (ImGui::InputText("##SourceDirectory", m_sourceDirectoryBuffer, sizeof(m_sourceDirectoryBuffer))) {
            m_project.config().sourceDirectory = m_sourceDirectoryBuffer;
        }
        // Keep file controls on their own row. The old SameLine layout could
        // push Browse completely outside a narrow two-column builder window.
        if (ImGui::Button("Browse Files...")) {
            const auto files = builder::browseSourceFiles();
            if (files.empty()) appendLog("[Assets] File picker cancelled or is unavailable. You can drag files onto the builder window.");
            for (const auto& file : files) addSourceFile(file);
        }
        ImGui::SameLine();
        if (ImGui::Button("Rescan Assets")) discoverAssets();
        ImGui::Spacing();
        ImGui::Text("SWF File:"); ImGui::InputText("##SWFPath", m_swfPathBuffer, sizeof(m_swfPathBuffer), ImGuiInputTextFlags_ReadOnly);
        ImGui::Text("IPA File (Optional):"); ImGui::InputText("##IPAPath", m_ipaPathBuffer, sizeof(m_ipaPathBuffer), ImGuiInputTextFlags_ReadOnly);
        if (m_project.hasValidSwf()) ImGui::TextColored(ImVec4(0.2f,1.0f,0.2f,1.0f), "[OK] Base SWF found.");
        else ImGui::TextColored(ImVec4(1.0f,0.7f,0.2f,1.0f), "[!] No SWF found. Browse or drop one here.");
        if (m_project.hasValidIpa()) ImGui::TextColored(ImVec4(0.2f,1.0f,0.2f,1.0f), "[OK] Optional IPA found.");
        ImGui::Spacing();
        ImGui::BeginChild("DropZone", ImVec2(0,64), true);
        ImGui::SetCursorPosX((ImGui::GetWindowWidth()-ImGui::CalcTextSize("Drop .SWF / .IPA files here").x)*0.5f);
        ImGui::SetCursorPosY(20.0f); ImGui::TextDisabled("Drop .SWF / .IPA files here"); ImGui::EndChild();
    }
}

void BuilderUI::renderFeaturesSection() {
    if (ImGui::CollapsingHeader("2. Detected & Enabled Features", ImGuiTreeNodeFlags_DefaultOpen)) {
        static bool featBase=true,featTowers=true,featBloons=true,featMaps=true,featSounds=true,featAchievements=true;
        ImGui::Checkbox("Base Game",&featBase); ImGui::SameLine(); ImGui::Checkbox("Towers",&featTowers); ImGui::SameLine(); ImGui::Checkbox("Bloons",&featBloons);
        ImGui::Checkbox("Maps",&featMaps); ImGui::SameLine(); ImGui::Checkbox("Sounds",&featSounds); ImGui::SameLine(); ImGui::Checkbox("Achievements",&featAchievements);
        bool enableMobile=m_project.config().enableMobileContent;
        if(ImGui::Checkbox("Mobile-Exclusive Content (Beekeeper, mobile maps)",&enableMobile)) m_project.config().enableMobileContent=enableMobile;
    }
}

void BuilderUI::renderPlatformSection() {
    if(ImGui::CollapsingHeader("3. Target Platform",ImGuiTreeNodeFlags_DefaultOpen)){
        const auto& backends=PlatformRegistry::instance().backends();
        for(const auto& backend:backends){
            bool selected=m_project.config().targetPlatform==backend->name(), available=backend->isAvailable();
            if(ImGui::RadioButton(backend->name().c_str(),selected)) m_project.config().targetPlatform=backend->name();
            ImGui::SameLine(); ImGui::TextColored(available?ImVec4(0.2f,1.0f,0.2f,1.0f):ImVec4(1.0f,0.3f,0.3f,1.0f),available?"[OK Toolchain detected]":"[! Toolchain unavailable]");
            ImGui::Indent(); ImGui::TextDisabled("%s",backend->description().c_str()); ImGui::Unindent();
        }
    }
}

void BuilderUI::renderActionButtons(){
    ImGui::Separator();
    if(ImGui::Button("Import Assets",ImVec2(130,32))) triggerImport(); ImGui::SameLine();
    if(ImGui::Button("Build Game",ImVec2(130,32))) triggerBuild(); ImGui::SameLine();
    if(ImGui::Button("Save Project",ImVec2(110,32))){
        if(m_project.saveToFile("project.btd4proj")) appendLog("[Project] Saved configuration to project.btd4proj");
        else appendLog("[Project] Error saving project file.");
    }
}

void BuilderUI::discoverAssets(){
    const bool found=m_project.discoverSourceAssets();
    std::strncpy(m_swfPathBuffer,m_project.config().sourceSwf.c_str(),sizeof(m_swfPathBuffer)-1); m_swfPathBuffer[sizeof(m_swfPathBuffer)-1]='\0';
    std::strncpy(m_ipaPathBuffer,m_project.config().sourceIpa.c_str(),sizeof(m_ipaPathBuffer)-1); m_ipaPathBuffer[sizeof(m_ipaPathBuffer)-1]='\0';
    if(found){appendLog("[Assets] Discovered source SWF: "+m_project.config().sourceSwf);if(m_project.hasValidIpa())appendLog("[Assets] Discovered optional IPA: "+m_project.config().sourceIpa);}
    else appendLog("[Assets] No SWF found in "+m_project.config().sourceDirectory+".");
}

void BuilderUI::triggerBuild(){
    if(!m_project.hasValidSwf()) discoverAssets();
    if(!m_project.hasValidSwf()){appendLog("[Build Error] No SWF source is selected. Import source assets first.");return;}
    // A build always refreshes the target data first. Never package stale or partial imports.
    if(!triggerImport()){appendLog("[Build Error] Asset import did not complete; build aborted before compilation/package.");return;}
    PlatformBackend* backend=PlatformRegistry::instance().findBackend(m_project.config().targetPlatform);
    if(!backend){appendLog("[Build Error] Unknown target platform: "+m_project.config().targetPlatform);return;}
    appendLog("========================================="); appendLog("[Build] Initiating playable build for: "+backend->name());
    if(!backend->isAvailable()){appendLog("[Build Error] "+backend->name()+" toolchain is not available on this host.");return;}
    BuildResult cfgRes=backend->configure(); for(const auto& line:cfgRes.outputLogs)appendLog("  "+line);
    if(!cfgRes.success){appendLog("[Build Error] Configuration failed: "+cfgRes.message);return;}
    BuildResult bldRes=backend->build(); for(const auto& line:bldRes.outputLogs)appendLog("  "+line);
    if(!bldRes.success){appendLog("[Build Error] Compilation failed: "+bldRes.message);return;}
    BuildResult pkgRes=backend->package(m_project.config().gameEdition); for(const auto& line:pkgRes.outputLogs)appendLog("  "+line);
    if(!pkgRes.success){appendLog("[Build Error] Packaging failed: "+pkgRes.message);return;}
    appendLog("[Build Success] Playable "+backend->name()+" build created.");
    appendLog("[Build] Run the executable from the Playable directory."); appendLog("=========================================");
}

bool BuilderUI::triggerImport(){
    appendLog("========================================="); appendLog("[Pipeline] Initiating BTD4 Asset Import Pipeline...");
    if(!m_project.hasValidSwf()) discoverAssets();
    if(!m_project.hasValidSwf()){appendLog("[Error] No SWF file found. Browse or drop the base game SWF into the builder.");return false;}
    tools::ImportOptions options;
    options.sourceSwf=m_project.config().sourceSwf; options.sourceIpa=m_project.config().sourceIpa;
    options.outputDir=std::string("game_data/")+m_project.config().targetPlatform+"/"+m_project.config().gameEdition;
    options.targetPlatform=m_project.config().targetPlatform;
    appendLog("[Pipeline] Game edition: "+m_project.config().gameEdition); appendLog("[Pipeline] Import target: "+options.targetPlatform); appendLog("[Pipeline] Output: "+options.outputDir);
    tools::ImportReport report=tools::AssetImporter::run(options,[this](const std::string& msg){appendLog(msg);});
    if(!report.success){appendLog("[Pipeline Error] Import failed: "+report.errorMessage);appendLog("=========================================");return false;}
    try{
        namespace fs=std::filesystem;
        const fs::path outputRounds=fs::path(options.outputDir)/"rounds"/"default_rounds.json";
        if(!fs::exists(outputRounds)){
            fs::copy_file("assets/placeholder/rounds/default_rounds.json",outputRounds,fs::copy_options::overwrite_existing);
            appendLog("[Pipeline] Added internal fallback round data; imported assets remain source-specific.");
        }
    }catch(const std::exception& e){appendLog(std::string("[Pipeline Warning] Could not add fallback round data: ")+e.what());}
    appendLog("[Pipeline Success] Successfully imported assets!"); appendLog("  Source family: "+report.sourceFamily);
    appendLog("  BTD4 detected: "+std::string(report.btd4Detected?"yes":"no")); appendLog("  IPA detected: "+std::string(report.ipaDetected?"yes":"no"));
    appendLog("  Textures extracted: "+std::to_string(report.texturesExtracted)); appendLog("  Audio cues extracted: "+std::to_string(report.soundsExtracted));
    appendLog("  Symbols mapped: "+std::to_string(report.symbolsMapped)); appendLog("  Manifest generated: "+report.manifestPath); appendLog("=========================================");
    return true;
}

void BuilderUI::renderLogsSection(){
    ImGui::Text("Build & Toolchain Logs"); ImGui::SameLine(ImGui::GetColumnWidth()-90); if(ImGui::Button("Clear Logs"))clearLogs(); ImGui::Separator();
    ImGui::BeginChild("LogScrollingRegion",ImVec2(0,0),true,ImGuiWindowFlags_HorizontalScrollbar);
    for(const auto& log:m_logs){
        if(log.find("[Build Error]")!=std::string::npos||log.find("[Error]")!=std::string::npos)ImGui::TextColored(ImVec4(1,0.4f,0.4f,1),"%s",log.c_str());
        else if(log.find("[Build Success]")!=std::string::npos||log.find("[OK]")!=std::string::npos)ImGui::TextColored(ImVec4(0.4f,1,0.4f,1),"%s",log.c_str());
        else ImGui::TextUnformatted(log.c_str());
    }
    if(m_autoScrollLogs&&ImGui::GetScrollY()>=ImGui::GetScrollMaxY())ImGui::SetScrollHereY(1.0f); ImGui::EndChild();
}

} // namespace btd4
