#include "FileDialog.hpp"

#include <cstdlib>
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#endif

namespace btd4::builder {

#ifdef _WIN32
static std::string utf8FromWide(const wchar_t* value) {
    if (!value) return {};
    const int length = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1) return {};
    std::string result(static_cast<size_t>(length - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), length, nullptr, nullptr);
    return result;
}
#endif

std::vector<std::string> browseSourceFiles() {
#ifdef _WIN32
    // A generous buffer lets the Windows picker return multiple files while
    // still keeping this dependency entirely native to the builder.
    std::vector<wchar_t> buffer(64 * 1024, L'\0');
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = nullptr;
    dialog.lpstrFilter = L"BTD4 Source Files\0*.swf;*.ipa\0Flash SWF\0*.swf\0iOS IPA\0*.ipa\0All Files\0*.*\0";
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY;
    dialog.lpstrTitle = L"Select Bloons TD 4 source files";

    if (!GetOpenFileNameW(&dialog)) return {};

    std::vector<std::string> result;
    if (buffer[dialog.nFileOffset - 1] == L'\0') {
        result.push_back(utf8FromWide(buffer.data()));
        return result;
    }

    // Explorer multi-select returns the directory first, followed by one or
    // more filenames, all separated by NULs.
    const wchar_t* directory = buffer.data();
    const wchar_t* name = buffer.data() + dialog.nFileOffset;
    std::filesystem::path root(directory);
    while (*name) {
        result.push_back((root / name).string());
        name += wcslen(name) + 1;
    }
    return result;
#elif defined(__linux__)
    // Keep the builder dependency-free. Most Linux desktops provide either
    // zenity or kdialog; try both before falling back to drag-and-drop.
    const char* commands[] = {
        "zenity --file-selection --multiple --separator='\\n' --file-filter='Source files | *.swf *.ipa' --title='Select Bloons TD 4 source files' 2>/dev/null",
        "kdialog --getopenfilename . '*.swf *.ipa' --multiple 2>/dev/null"
    };

    for (const char* command : commands) {
        FILE* pipe = popen(command, "r");
        if (!pipe) continue;
        std::vector<std::string> result;
        char line[4096];
        while (fgets(line, sizeof(line), pipe)) {
            std::string path(line);
            while (!path.empty() && (path.back() == '\n' || path.back() == '\r')) path.pop_back();
            if (!path.empty()) result.push_back(path);
        }
        const int status = pclose(pipe);
        if (status == 0 && !result.empty()) return result;
    }
#endif
    return {};
}

} // namespace btd4::builder
