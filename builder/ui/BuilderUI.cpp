#include "BuilderUI.hpp"
#include "../../tools/asset_importer/AssetImporter.hpp"
#include "../platform/FileDialog.hpp"
#include <imgui.h>
#include <cstring>
#include <algorithm>
#include <cstddef>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#endif
#include <filesystem>
#include <exception>

namespace btd4 {
namespace fs = std::filesystem;

namespace {

bool isProjectRoot(const fs::path& candidate) {
    std::error_code ec;
    return fs::is_regular_file(candidate / "CMakeLists.txt", ec) &&
           fs::is_regular_file(candidate / "engine" / "core" / "Engine.cpp", ec) &&
           fs::is_regular_file(candidate / "platform" / "common" / "PlatformRegistry.cpp", ec);
}

void setSourceRootEnvironment(const fs::path& root) {
#ifdef _WIN32
    _putenv_s("BTD4_SOURCE_ROOT", root.string().c_str());
#else
    setenv("BTD4_SOURCE_ROOT", root.string().c_str(), 1);
#endif
}

fs::path findProjectRoot() {
    std::error_code ec;

    auto searchUpward = [&](fs::path current) -> fs::path {
        current = current.lexically_normal();
        for (int i = 0; i < 32 && !current.empty(); ++i) {
            if (isProjectRoot(current)) return current;
            const fs::path parent = current.parent_path();
            if (parent == current) break;
            current = parent;
        }
        return {};
    };

    if (const char* configuredRoot = std::getenv("BTD4_SOURCE_ROOT")) {
        if (*configuredRoot) {
            const fs::path candidate(configuredRoot);
            if (isProjectRoot(candidate)) return candidate;
        }
    }

    if (const char* builderRoot = std::getenv("BTD4_BUILDER_ROOT")) {
        if (*builderRoot) {
            const fs::path found = searchUpward(fs::path(builderRoot));
            if (!found.empty()) {
                setSourceRootEnvironment(found);
                return found;
            }
        }
    }

    const fs::path current = fs::current_path(ec);
    if (!ec) {
        const fs::path found = searchUpward(current);
        if (!found.empty()) {
            setSourceRootEnvironment(found);
            return found;
        }
    }
    return {};
}

// The builder can also be distributed as a portable editor/importer bundle,
// where the source checkout and CMakeLists.txt are intentionally absent.
// Asset import and editor data still need a writable workspace in that mode.
fs::path findWorkspaceRoot() {
    if (const fs::path projectRoot = findProjectRoot(); !projectRoot.empty()) {
        return projectRoot;
    }

    if (const char* builderRoot = std::getenv("BTD4_BUILDER_ROOT")) {
        if (*builderRoot) return fs::path(builderRoot).lexically_normal();
    }

    std::error_code ec;
    const fs::path current = fs::current_path(ec);
    if (ec) return fs::path{};
    const fs::path workspace = current.lexically_normal();
#ifdef _WIN32
    if (std::getenv("BTD4_BUILDER_ROOT") == nullptr) {
        _putenv_s("BTD4_BUILDER_ROOT", workspace.string().c_str());
    }
#else
    if (std::getenv("BTD4_BUILDER_ROOT") == nullptr) {
        setenv("BTD4_BUILDER_ROOT", workspace.string().c_str(), 1);
    }
#endif
    return workspace;
}

std::string fileFingerprint(const fs::path& path) {
    std::error_code ec;
    const fs::path absolute = fs::absolute(path, ec).lexically_normal();
    if (ec || !fs::is_regular_file(absolute, ec)) return absolute.string();

    const auto size = fs::file_size(absolute, ec);
    if (ec) return absolute.string();
    ec.clear();
    const auto timestamp = fs::last_write_time(absolute, ec);
    if (ec) return absolute.string() + "|" + std::to_string(size);

    return absolute.string() + "|" + std::to_string(size) + "|" +
           std::to_string(timestamp.time_since_epoch().count());
}

std::string makeImportKey(const Project& project) {
    std::string key;
    key.reserve(project.config().sourceSwf.size() + project.config().sourceIpa.size() + project.config().sourceExpansionSwf.size() + project.config().sourceHdIpa.size() + project.config().sourceMobileIpa.size() + 256);
    key += fileFingerprint(project.config().sourceSwf);
    key += '\n';
    if (!project.config().sourceIpa.empty()) key += fileFingerprint(project.config().sourceIpa);
    key += '\n' + fileFingerprint(project.config().sourceExpansionSwf);
    key += '\n' + fileFingerprint(project.config().sourceHdIpa);
    key += '\n' + fileFingerprint(project.config().sourceMobileIpa);
    key += '\n';
    key += project.config().gameEdition;
    key += '\n';
    key += project.config().targetPlatform;
    return key;
}

fs::path findImporterExecutable() {
    std::error_code ec;
    fs::path root;
    if (const char* envRoot = std::getenv("BTD4_BUILDER_ROOT")) {
        if (*envRoot) root = fs::path(envRoot);
    }
    if (root.empty()) root = fs::current_path(ec);

#ifdef _WIN32
    const fs::path candidate = root / "btd4_importer.exe";
#else
    const fs::path candidate = root / "btd4_importer";
#endif
    if (fs::is_regular_file(candidate, ec)) return candidate;
    return {};
}

std::string readTextFile(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) return {};
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

std::string shellQuote(const std::string& value) {
    std::string result = "'";
    for (char c : value) {
        if (c == '\'') result += "'\\''";
        else result += c;
    }
    result += "'";
    return result;
}

#ifdef _WIN32
std::wstring utf8ToWide(const std::string& value) {
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) return {};
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::wstring quoteWindowsArg(const std::wstring& value) {
    std::wstring result = L"\"";
    size_t backslashes = 0;
    for (wchar_t c : value) {
        if (c == L'\\') {
            ++backslashes;
            continue;
        }
        if (c == L'"') {
            result.append(backslashes * 2 + 1, L'\\');
            result += L'"';
            backslashes = 0;
            continue;
        }
        result.append(backslashes, L'\\');
        result += c;
        backslashes = 0;
    }
    result.append(backslashes * 2, L'\\');
    result += L'"';
    return result;
}
#endif

int runStandaloneImporter(const fs::path& importer,
                           const fs::path& sourceSwf,
                           const fs::path& sourceIpa,
                           const fs::path& sourceExpansionSwf,
                           const fs::path& sourceHdIpa,
                           const fs::path& sourceMobileIpa,
                           const fs::path& outputDir,
                           const fs::path& projectRoot,
                           const std::string& targetPlatform,
                           const std::string& gameEdition,
                           std::string& outputLog) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    std::error_code tempEc;
    fs::path tempRoot = fs::temp_directory_path(tempEc);
    if (tempEc || tempRoot.empty()) {
        tempRoot = projectRoot / "builds";
        tempEc.clear();
    }
    fs::create_directories(tempRoot, tempEc);
    if (tempEc) {
        outputLog = "Could not create temporary importer log directory: " + tempEc.message();
        return -1;
    }
    const fs::path logPath = tempRoot /
        ("btd4_builder_import_" + std::to_string(stamp) + ".log");

#ifdef _WIN32
    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.bInheritHandle = TRUE;
    HANDLE logHandle = CreateFileW(
        logPath.wstring().c_str(), GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, &securityAttributes, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (logHandle == INVALID_HANDLE_VALUE) {
        outputLog = "Could not create temporary importer log.";
        return -1;
    }

    std::wstring commandLine = quoteWindowsArg(utf8ToWide(importer.string()));
    commandLine += L" " + quoteWindowsArg(utf8ToWide(sourceSwf.string()));
    commandLine += L" --out " + quoteWindowsArg(utf8ToWide(outputDir.string()));
    commandLine += L" --platform " + quoteWindowsArg(utf8ToWide(targetPlatform));
    commandLine += L" --edition " + quoteWindowsArg(utf8ToWide(gameEdition));
    if (!sourceIpa.empty()) commandLine += L" --ipa " + quoteWindowsArg(utf8ToWide(sourceIpa.string()));
    if (!sourceExpansionSwf.empty()) commandLine += L" --expansion-swf " + quoteWindowsArg(utf8ToWide(sourceExpansionSwf.string()));
    if (!sourceHdIpa.empty()) commandLine += L" --hd-ipa " + quoteWindowsArg(utf8ToWide(sourceHdIpa.string()));
    if (!sourceMobileIpa.empty()) commandLine += L" --mobile-ipa " + quoteWindowsArg(utf8ToWide(sourceMobileIpa.string()));

    std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = logHandle;
    startup.hStdError = logHandle;

    PROCESS_INFORMATION process{};
    const std::wstring workingDirectory = projectRoot.wstring();
    const BOOL created = CreateProcessW(
        nullptr, mutableCommand.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, workingDirectory.c_str(), &startup, &process);

    CloseHandle(logHandle);

    if (!created) {
        outputLog = "Could not launch standalone asset importer. Windows error " +
                    std::to_string(GetLastError()) + ".";
        return -1;
    }

    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    outputLog = readTextFile(logPath);
    std::error_code ec;
    fs::remove(logPath, ec);
    return static_cast<int>(exitCode);
#else
    const std::string command =
        shellQuote(importer.string()) + " " +
        shellQuote(sourceSwf.string()) +
        " --out " + shellQuote(outputDir.string()) +
        " --platform " + shellQuote(targetPlatform) +
        (sourceIpa.empty() ? std::string{} : " --ipa " + shellQuote(sourceIpa.string())) +
        (sourceExpansionSwf.empty() ? std::string{} : " --expansion-swf " + shellQuote(sourceExpansionSwf.string())) +
        (sourceHdIpa.empty() ? std::string{} : " --hd-ipa " + shellQuote(sourceHdIpa.string())) +
        (sourceMobileIpa.empty() ? std::string{} : " --mobile-ipa " + shellQuote(sourceMobileIpa.string())) +
        " --edition " + shellQuote(gameEdition) +
        " > " + shellQuote(logPath.string()) + " 2>&1";

    const int status = std::system(command.c_str());
    outputLog = readTextFile(logPath);
    std::error_code ec;
    fs::remove(logPath, ec);
    return status == 0 ? 0 : status;
#endif
}

} // namespace

BuilderUI::BuilderUI() {
    appendLog("[Builder] Initialized Game Builder UI.");
    appendLog("[Builder] Ready to configure project and build targets.");
}

void BuilderUI::initialize() {
    m_mapEditor.initialize();
    m_roundEditor.initialize();
    m_towerEditor.initialize();
    m_upgradeEditor.initialize();
    const fs::path projectRoot = findWorkspaceRoot();
    if (!projectRoot.empty()) {
        const fs::path projectFile = projectRoot / "project.btd4proj";
        std::error_code ec;
        if (fs::is_regular_file(projectFile, ec) && m_project.loadFromFile(projectFile.string())) {
            appendLog("[Project] Loaded configuration from " + projectFile.string());
        }
    }

    std::strncpy(m_sourceDirectoryBuffer, m_project.config().sourceDirectory.c_str(), sizeof(m_sourceDirectoryBuffer) - 1);
    m_sourceDirectoryBuffer[sizeof(m_sourceDirectoryBuffer) - 1] = '\0';
    std::strncpy(m_swfPathBuffer, m_project.config().sourceSwf.c_str(), sizeof(m_swfPathBuffer) - 1);
    m_swfPathBuffer[sizeof(m_swfPathBuffer) - 1] = '\0';
    std::strncpy(m_ipaPathBuffer, m_project.config().sourceIpa.c_str(), sizeof(m_ipaPathBuffer) - 1);
    std::strncpy(m_expansionSwfPathBuffer, m_project.config().sourceExpansionSwf.c_str(), sizeof(m_expansionSwfPathBuffer) - 1);
    std::strncpy(m_hdIpaPathBuffer, m_project.config().sourceHdIpa.c_str(), sizeof(m_hdIpaPathBuffer) - 1);
    std::strncpy(m_mobileIpaPathBuffer, m_project.config().sourceMobileIpa.c_str(), sizeof(m_mobileIpaPathBuffer) - 1);
    m_ipaPathBuffer[sizeof(m_ipaPathBuffer) - 1] = '\0';
    discoverAssets();
}

void BuilderUI::appendLog(const std::string& line) {
    constexpr std::size_t kMaxLogEntries = 5000;
    constexpr std::size_t kTrimLogEntries = 1000;
    if (m_logs.size() >= kMaxLogEntries) {
        const auto trimCount = (std::min)(kTrimLogEntries, m_logs.size());
        m_logs.erase(m_logs.begin(), m_logs.begin() + static_cast<std::ptrdiff_t>(trimCount));
    }
    m_logs.push_back(line);
}

void BuilderUI::clearLogs() {
    m_logs.clear();
}

void BuilderUI::addSourceFile(const std::string& path) {
    if (m_project.addSourceFile(path)) {
        std::strncpy(m_swfPathBuffer, m_project.config().sourceSwf.c_str(), sizeof(m_swfPathBuffer) - 1);
        m_swfPathBuffer[sizeof(m_swfPathBuffer) - 1] = '\0';
        std::strncpy(m_ipaPathBuffer, m_project.config().sourceIpa.c_str(), sizeof(m_ipaPathBuffer) - 1);
        m_ipaPathBuffer[sizeof(m_ipaPathBuffer) - 1] = '\0';
        appendLog("[Assets] Added source file: " + path);
    } else {
        appendLog("[Assets] Ignored unsupported or missing file: " + path);
    }
}

void BuilderUI::render() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("BTD4 Game Builder", nullptr, flags);

    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "BLOONS TD 4 REPOPPED - GAME BUILDER");
    ImGui::Separator();

    if (ImGui::BeginTable("BuilderMainTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextColumn();
        renderSourceFilesSection();
        ImGui::Spacing();
        renderFeaturesSection();
        ImGui::Spacing();
        renderPlatformSection();
        ImGui::Spacing();
        const fs::path projectRoot = findWorkspaceRoot();
        m_mapEditor.render(projectRoot.string());
        ImGui::Spacing();
        m_roundEditor.render(projectRoot.string(), m_mapEditor.map());
        ImGui::Spacing();
        m_towerEditor.render(projectRoot.string());
        ImGui::Spacing();
        m_upgradeEditor.render(projectRoot.string());
        ImGui::Spacing();
        renderActionButtons();

        ImGui::TableNextColumn();
        renderLogsSection();
        ImGui::EndTable();
    }

    ImGui::End();
}

void BuilderUI::renderSourceFilesSection() {
    if (!ImGui::CollapsingHeader("1. Source Game Files", ImGuiTreeNodeFlags_DefaultOpen)) return;

    ImGui::TextDisabled("Select or drop your own source files. Nothing is copied into the repository.");
    ImGui::TextDisabled("Accepted: .SWF for Flash/Expansion, .IPA for HD/mobile content.");
    ImGui::Spacing();

    ImGui::Text("Game Edition:");
    const char* editions[] = { "BTD4 Flash", "BTD4 Expansion", "BTD4 HD (iPad)", "Definitive Edition" };
    int editionIndex = 0;
    for (int i = 0; i < 4; ++i) {
        if (m_project.config().gameEdition == editions[i]) editionIndex = i;
    }
    if (ImGui::Combo("##GameEdition", &editionIndex, editions, 4)) {
        m_project.config().gameEdition = editions[editionIndex];
        if (editionIndex == 2) m_project.config().enableMobileContent = true;
        appendLog("[Project] Game edition: " + m_project.config().gameEdition);
    }

    if (editionIndex == 0) ImGui::TextDisabled("Original Flash release. Source-specific maps/content are kept separate.");
    if (editionIndex == 1) ImGui::TextDisabled("Expansion release. Exclusive expansion content is treated as its own source set.");
    if (editionIndex == 2) ImGui::TextDisabled("iPad HD release. Mobile-only maps/assets can be imported without replacing Flash content.");
    if (editionIndex == 3) ImGui::TextDisabled("Definitive Edition: merges Flash, Expansion, phone/mobile and HD sources with target-aware quality selection.");

    ImGui::Spacing();
    ImGui::Text("Source Asset Folder:");
    ImGui::TextDisabled("This controls source-file discovery only. Imported playable data is written to game_data.");
    if (ImGui::InputText("##SourceDirectory", m_sourceDirectoryBuffer, sizeof(m_sourceDirectoryBuffer))) {
        m_project.config().sourceDirectory = m_sourceDirectoryBuffer;
    }

    if (ImGui::Button("Browse Files...")) {
        const auto files = builder::browseSourceFiles();
        if (files.empty()) {
            appendLog("[Assets] File picker cancelled or is unavailable. You can drag files onto the builder window.");
        }
        for (const auto& file : files) addSourceFile(file);
    }
    ImGui::SameLine();
    if (ImGui::Button("Rescan Assets")) discoverAssets();

    ImGui::Spacing();
    ImGui::Text("SWF File:");
    ImGui::InputText("##SWFPath", m_swfPathBuffer, sizeof(m_swfPathBuffer), ImGuiInputTextFlags_ReadOnly);
    ImGui::Text("Primary IPA / Phone File (Optional):");
    ImGui::InputText("##IPAPath", m_ipaPathBuffer, sizeof(m_ipaPathBuffer), ImGuiInputTextFlags_ReadOnly);
    ImGui::Text("Expansion SWF (Optional):");
    ImGui::InputText("##ExpansionSWFPath", m_expansionSwfPathBuffer, sizeof(m_expansionSwfPathBuffer), ImGuiInputTextFlags_ReadOnly);
    ImGui::Text("HD iPad IPA (Optional):");
    ImGui::InputText("##HDIPAPath", m_hdIpaPathBuffer, sizeof(m_hdIpaPathBuffer), ImGuiInputTextFlags_ReadOnly);
    ImGui::Text("Phone/Mobile IPA (Optional):");
    ImGui::InputText("##MobileIPAPath", m_mobileIpaPathBuffer, sizeof(m_mobileIpaPathBuffer), ImGuiInputTextFlags_ReadOnly);

    if (m_project.hasValidSwf()) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[OK] Base SWF found.");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "[!] No SWF found. Browse or drop one here.");
    }
    if (m_project.hasValidIpa()) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[OK] Optional IPA found.");
    }

    ImGui::Spacing();
    ImGui::BeginChild("DropZone", ImVec2(0, 64), true);
    const char* text = "Drop .SWF / .IPA files here (multiple sources supported)";
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) * 0.5f);
    ImGui::SetCursorPosY(20.0f);
    ImGui::TextDisabled("%s", text);
    ImGui::EndChild();
}

void BuilderUI::renderFeaturesSection() {
    if (!ImGui::CollapsingHeader("2. Detected & Enabled Features", ImGuiTreeNodeFlags_DefaultOpen)) return;

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

void BuilderUI::renderPlatformSection() {
    if (!ImGui::CollapsingHeader("3. Target Platform", ImGuiTreeNodeFlags_DefaultOpen)) return;

    const auto& backends = PlatformRegistry::instance().backends();
    for (const auto& backend : backends) {
        const bool selected = m_project.config().targetPlatform == backend->name();
        const bool available = backend->isAvailable();
        if (ImGui::RadioButton(backend->name().c_str(), selected)) {
            m_project.config().targetPlatform = backend->name();
        }
        ImGui::SameLine();
        ImGui::TextColored(
            available ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            "%s",
            available ? "[OK Toolchain detected]" : "[! Toolchain unavailable]"
        );
        ImGui::Indent();
        ImGui::TextDisabled("%s", backend->description().c_str());
        ImGui::Unindent();
    }
}

void BuilderUI::renderActionButtons() {
    ImGui::Separator();

    if (ImGui::Button("New Project", ImVec2(110, 32))) {
        m_project = Project();
        m_mapEditor.initialize();
        m_roundEditor.initialize();
        m_towerEditor.initialize();
        m_upgradeEditor.initialize();
        m_lastSuccessfulImportKey.clear();
        std::strncpy(m_sourceDirectoryBuffer, m_project.config().sourceDirectory.c_str(), sizeof(m_sourceDirectoryBuffer) - 1);
        m_sourceDirectoryBuffer[sizeof(m_sourceDirectoryBuffer) - 1] = '\0';
        m_swfPathBuffer[0] = '\0';
        m_ipaPathBuffer[0] = '\0';
        m_expansionSwfPathBuffer[0] = '\0';
        m_hdIpaPathBuffer[0] = '\0';
        m_mobileIpaPathBuffer[0] = '\0';
        appendLog("[Project] Created a new empty project.");
    }
    ImGui::SameLine();

    if (ImGui::Button("Validate Project", ImVec2(120, 32))) validateProject();
    ImGui::SameLine();
    if (ImGui::Button("Preview Build", ImVec2(120, 32))) previewBuild();
    ImGui::SameLine();
    if (ImGui::Button("Load Project", ImVec2(110, 32))) {
        const fs::path projectRoot = findWorkspaceRoot();
        const fs::path loadPath = projectRoot.empty() ? fs::path("project.btd4proj") : (projectRoot / "project.btd4proj");
        if (m_project.loadFromFile(loadPath.string())) {
            std::strncpy(m_sourceDirectoryBuffer, m_project.config().sourceDirectory.c_str(), sizeof(m_sourceDirectoryBuffer) - 1);
            m_sourceDirectoryBuffer[sizeof(m_sourceDirectoryBuffer) - 1] = '\0';
            std::strncpy(m_swfPathBuffer, m_project.config().sourceSwf.c_str(), sizeof(m_swfPathBuffer) - 1);
            m_swfPathBuffer[sizeof(m_swfPathBuffer) - 1] = '\0';
            std::strncpy(m_ipaPathBuffer, m_project.config().sourceIpa.c_str(), sizeof(m_ipaPathBuffer) - 1);
            m_ipaPathBuffer[sizeof(m_ipaPathBuffer) - 1] = '\0';
            std::strncpy(m_expansionSwfPathBuffer, m_project.config().sourceExpansionSwf.c_str(), sizeof(m_expansionSwfPathBuffer) - 1);
            std::strncpy(m_hdIpaPathBuffer, m_project.config().sourceHdIpa.c_str(), sizeof(m_hdIpaPathBuffer) - 1);
            std::strncpy(m_mobileIpaPathBuffer, m_project.config().sourceMobileIpa.c_str(), sizeof(m_mobileIpaPathBuffer) - 1);
            m_lastSuccessfulImportKey.clear();
            appendLog("[Project] Loaded configuration from " + loadPath.string());
            discoverAssets();
        } else {
            appendLog("[Project] No readable project file found at " + loadPath.string());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Assets", ImVec2(120, 32))) triggerImport();
    ImGui::SameLine();
    if (ImGui::Button("Build Game", ImVec2(120, 32))) triggerBuild();
    ImGui::SameLine();
    if (ImGui::Button("Save Project", ImVec2(110, 32))) {
        const fs::path projectRoot = findWorkspaceRoot();
        const fs::path savePath = projectRoot.empty() ? fs::path("project.btd4proj") : (projectRoot / "project.btd4proj");
        if (m_project.saveToFile(savePath.string())) {
            appendLog("[Project] Saved configuration to " + savePath.string());
        } else {
            appendLog("[Project] Error saving project file.");
        }
    }
}

void BuilderUI::discoverAssets() {
    bool found = m_project.hasValidSwf();
    if (!found) {
        found = m_project.discoverSourceAssets();
    } else if (m_project.hasValidIpa()) {
        appendLog("[Assets] Preserving project-configured source files.");
    }

    std::strncpy(m_swfPathBuffer, m_project.config().sourceSwf.c_str(), sizeof(m_swfPathBuffer) - 1);
    m_swfPathBuffer[sizeof(m_swfPathBuffer) - 1] = '\0';
    std::strncpy(m_ipaPathBuffer, m_project.config().sourceIpa.c_str(), sizeof(m_ipaPathBuffer) - 1);
    m_ipaPathBuffer[sizeof(m_ipaPathBuffer) - 1] = '\0';

    if (found) {
        appendLog("[Assets] Source SWF ready: " + m_project.config().sourceSwf);
        if (m_project.hasValidIpa()) appendLog("[Assets] Optional IPA ready: " + m_project.config().sourceIpa);
        if (!m_project.config().sourceExpansionSwf.empty()) appendLog("[Assets] Expansion SWF ready: " + m_project.config().sourceExpansionSwf);
        if (!m_project.config().sourceHdIpa.empty()) appendLog("[Assets] HD iPad IPA ready: " + m_project.config().sourceHdIpa);
        if (!m_project.config().sourceMobileIpa.empty()) appendLog("[Assets] Phone/mobile IPA ready: " + m_project.config().sourceMobileIpa);
    } else {
        appendLog("[Assets] No SWF found in " + m_project.config().sourceDirectory + ".");
    }
}

bool BuilderUI::validateProject() {
    bool valid = true;
    const auto& config = m_project.config();

    if (config.projectName.empty()) {
        appendLog("[Validation Error] Project name is empty.");
        valid = false;
    }
    if (!m_project.hasValidSwf()) {
        appendLog("[Validation Error] No valid SWF source file is configured.");
        valid = false;
    } else {
        std::error_code ec;
        if (!fs::is_regular_file(fs::path(config.sourceSwf), ec)) {
            appendLog("[Validation Error] Configured SWF does not exist: " + config.sourceSwf);
            valid = false;
        }
    }
    if (!config.sourceIpa.empty()) {
        std::error_code ec;
        if (!fs::is_regular_file(fs::path(config.sourceIpa), ec)) {
            appendLog("[Validation Error] Configured IPA does not exist: " + config.sourceIpa);
            valid = false;
        }
    }

    static const char* editions[] = {"BTD4 Flash", "BTD4 Expansion", "BTD4 HD (iPad)", "Definitive Edition"};
    bool knownEdition = false;
    for (const char* edition : editions) {
        if (config.gameEdition == edition) {
            knownEdition = true;
            break;
        }
    }
    if (!knownEdition) {
        appendLog("[Validation Error] Unknown game edition: " + config.gameEdition);
        valid = false;
    }

    if (PlatformRegistry::instance().findBackend(config.targetPlatform) == nullptr) {
        appendLog("[Validation Error] Unknown target platform: " + config.targetPlatform);
        valid = false;
    }

    if (config.buildConfiguration.empty()) {
        appendLog("[Validation Error] Build configuration is empty.");
        valid = false;
    }

    appendLog(valid ? "[Validation] Project configuration is valid."
                    : "[Validation] Project configuration has errors.");
    return valid;
}

void BuilderUI::previewBuild() {
    const fs::path projectRoot = findWorkspaceRoot();
    if (projectRoot.empty()) {
        appendLog("[Preview Error] Could not locate the builder workspace.");
        return;
    }

    fs::path packageDir;
    if (m_project.config().targetPlatform == "Windows") {
        packageDir = projectRoot / "builds" / "Windows" / "Playable";
        if (!fs::exists(packageDir / "btd4_game.exe")) {
            packageDir = projectRoot / "Playable";
        }
    } else if (m_project.config().targetPlatform == "Linux") {
        packageDir = projectRoot / "builds" / "Linux" / "Playable";
    } else {
        appendLog("[Preview Error] Preview is currently available for Windows and Linux desktop builds only.");
        return;
    }

#ifdef _WIN32
    const fs::path executable = packageDir / "btd4_game.exe";
    if (!fs::is_regular_file(executable)) {
        appendLog("[Preview Error] No packaged Windows game was found. Build the game first.");
        return;
    }
    const std::string command =
        "start \"BTD4 Repopped Preview\" /D \"" + packageDir.string() +
        "\" \"" + executable.filename().string() + "\"";
#else
    const fs::path executable = packageDir / "btd4_game";
    if (!fs::is_regular_file(executable)) {
        appendLog("[Preview Error] No packaged Linux game was found. Build the game first.");
        return;
    }
    const std::string command =
        "cd \"" + packageDir.string() + "\" && \"" + executable.string() +
        "\" >/dev/null 2>&1 &";
#endif

    const int status = std::system(command.c_str());
    if (status != 0) {
        appendLog("[Preview Error] Could not launch packaged game. Exit code " + std::to_string(status) + ".");
        return;
    }
    appendLog("[Preview] Launched " + executable.string());
}

void BuilderUI::triggerBuild() {
    if (!m_project.hasValidSwf()) discoverAssets();
    if (!validateProject()) {
        appendLog("[Build Error] Project validation failed; build aborted.");
        return;
    }
    if (!m_project.hasValidSwf()) {
        appendLog("[Build Error] No SWF source is selected. Import source assets first.");
        return;
    }

    if (!triggerImport()) {
        appendLog("[Build Error] Asset import did not complete; build aborted before compilation/package.");
        return;
    }

    PlatformBackend* backend = PlatformRegistry::instance().findBackend(m_project.config().targetPlatform);
    if (!backend) {
        appendLog("[Build Error] Unknown target platform: " + m_project.config().targetPlatform);
        return;
    }

    appendLog("=========================================");
    appendLog("[Build] Initiating playable build for: " + backend->name());

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

    BuildResult pkgRes = backend->package(m_project.config().gameEdition);
    for (const auto& line : pkgRes.outputLogs) appendLog("  " + line);
    if (!pkgRes.success) {
        appendLog("[Build Error] Packaging failed: " + pkgRes.message);
        return;
    }

    appendLog("[Build Success] Playable " + backend->name() + " build created.");
    appendLog("[Build] Run the executable from the Playable directory.");
    appendLog("=========================================");
}

bool BuilderUI::triggerImport() {
    appendLog("=========================================");
    appendLog("[Pipeline] Initiating BTD4 Asset Import Pipeline...");

    if (!m_project.hasValidSwf()) discoverAssets();
    if (!m_project.hasValidSwf()) {
        appendLog("[Error] No SWF file found. Browse or drop the base game SWF into the builder.");
        return false;
    }

    const std::string importKey = makeImportKey(m_project);
    if (importKey == m_lastSuccessfulImportKey) {
        appendLog("[Pipeline] These source assets are already imported for the selected edition/platform; skipping duplicate import.");
        appendLog("=========================================");
        return true;
    }

    const fs::path projectRoot = findWorkspaceRoot();
    if (projectRoot.empty()) {
        appendLog("[Pipeline Error] Could not determine a writable builder workspace.");
        appendLog("[Pipeline Error] Run the builder from its packaged folder or a project checkout.");
        appendLog("=========================================");
        return false;
    }
    if (!isProjectRoot(projectRoot)) {
        appendLog("[Pipeline] No CMake project root was found; using the portable builder workspace for imported game_data.");
    }
    appendLog("[Pipeline] Workspace: " + projectRoot.string());

    tools::ImportOptions options;
    options.sourceSwf = m_project.config().sourceSwf;
    options.sourceIpa = m_project.config().sourceIpa;
    options.sourceExpansionSwf = m_project.config().sourceExpansionSwf;
    options.sourceHdIpa = m_project.config().sourceHdIpa;
    options.sourceMobileIpa = m_project.config().sourceMobileIpa;
    options.gameEdition = m_project.config().gameEdition;
    options.outputDir = (projectRoot / "game_data" / m_project.config().targetPlatform / m_project.config().gameEdition).string();
    options.targetPlatform = m_project.config().targetPlatform;

    std::error_code outputEc;
    fs::create_directories(fs::path(options.outputDir), outputEc);
    if (outputEc) {
        appendLog("[Pipeline Error] Could not create import output directory: " + outputEc.message());
        appendLog("  Output: " + options.outputDir);
        appendLog("=========================================");
        return false;
    }

    appendLog("[Pipeline] Game edition: " + m_project.config().gameEdition);
    appendLog("[Pipeline] Import target: " + options.targetPlatform);
    appendLog("[Pipeline] Output: " + options.outputDir);

    const fs::path importerExecutable = findImporterExecutable();
    if (importerExecutable.empty()) {
        appendLog("[Pipeline Error] Standalone btd4_importer executable was not found next to the builder.");
        appendLog("[Pipeline Error] The importer must be packaged with btd4_builder.exe.");
        appendLog("=========================================");
        return false;
    }

    appendLog("[Pipeline] Running isolated asset importer: " + importerExecutable.string());
    std::string importerOutput;
    const int importerExitCode = runStandaloneImporter(
        importerExecutable,
        fs::path(options.sourceSwf),
        fs::path(options.sourceIpa),
        fs::path(options.sourceExpansionSwf),
        fs::path(options.sourceHdIpa),
        fs::path(options.sourceMobileIpa),
        fs::path(options.outputDir),
        projectRoot,
        options.targetPlatform,
        options.gameEdition,
        importerOutput);

    if (!importerOutput.empty()) {
        std::istringstream importerLines(importerOutput);
        std::string line;
        while (std::getline(importerLines, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) appendLog(line);
        }
    }

    const fs::path manifestPath = fs::path(options.outputDir) / "manifest.json";
    std::error_code manifestEc;
    if (importerExitCode != 0) {
        appendLog("[Pipeline Error] Standalone importer exited with code " + std::to_string(importerExitCode) + ".");
        if (importerExitCode == static_cast<int>(0xC0000005u)) {
            appendLog("[Pipeline Error] The importer hit a native access violation. The builder was kept alive by isolating the importer process.");
        }
        appendLog("=========================================");
        return false;
    }
    if (!fs::is_regular_file(manifestPath, manifestEc)) {
        appendLog("[Pipeline Error] Importer reported success, but manifest.json was not produced.");
        appendLog("=========================================");
        return false;
    }

    try {
        const fs::path outputRounds = fs::path(options.outputDir) / "rounds" / "default_rounds.json";
        if (!fs::exists(outputRounds)) {
            const fs::path fallbackRounds = projectRoot / "assets" / "placeholder" / "rounds" / "default_rounds.json";
            if (fs::exists(fallbackRounds)) {
                fs::copy_file(fallbackRounds, outputRounds, fs::copy_options::overwrite_existing);
                appendLog("[Pipeline] Added internal fallback round data; imported assets remain source-specific.");
            } else {
                appendLog("[Pipeline Warning] Placeholder round data was not found; runtime fallback will be used directly.");
            }
        }
    } catch (const std::exception& e) {
        appendLog(std::string("[Pipeline Warning] Could not add fallback round data: ") + e.what());
    }

    m_lastSuccessfulImportKey = importKey;
    appendLog("[Pipeline Success] Successfully imported assets!");
    appendLog("  Manifest generated: " + manifestPath.string());
    appendLog("  The isolated importer completed successfully; its detailed summary is included above.");
    appendLog("=========================================");
    return true;
}

void BuilderUI::renderLogsSection() {
    ImGui::Text("Build & Toolchain Logs");
    ImGui::SameLine(ImGui::GetColumnWidth() - 90);
    if (ImGui::Button("Clear Logs")) clearLogs();
    ImGui::Separator();

    ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& log : m_logs) {
        if (log.find("[Build Error]") != std::string::npos || log.find("[Error]") != std::string::npos) {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", log.c_str());
        } else if (log.find("[Build Success]") != std::string::npos || log.find("[OK]") != std::string::npos) {
            ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), "%s", log.c_str());
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
