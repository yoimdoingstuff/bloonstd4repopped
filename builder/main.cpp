#define SDL_MAIN_HANDLED
#include "ui/BuilderUI.hpp"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
void setBuilderRootEnvironment(const std::filesystem::path& root) {
#ifdef _WIN32
    _putenv_s("BTD4_BUILDER_ROOT", root.string().c_str());
#else
    setenv("BTD4_BUILDER_ROOT", root.string().c_str(), 1);
#endif
}

std::filesystem::path findExecutableRoot() {
#ifdef _WIN32
    std::wstring buffer(512, L'\0');
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) return {};
        if (length < buffer.size() - 1) {
            buffer.resize(length);
            return std::filesystem::path(buffer).parent_path().lexically_normal();
        }
        buffer.resize(buffer.size() * 2);
        if (buffer.size() > 32768) return {};
    }
#else
    if (char* basePath = SDL_GetBasePath()) {
        std::filesystem::path root = std::filesystem::path(basePath).lexically_normal();
        SDL_free(basePath);
        return root;
    }
    return {};
#endif
}
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    std::filesystem::path executableRoot = findExecutableRoot();
    std::error_code pathError;
    if (!executableRoot.empty() && std::filesystem::is_directory(executableRoot, pathError)) {
#ifdef _WIN32
        if (!SetCurrentDirectoryW(executableRoot.wstring().c_str())) {
            pathError = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        }
#else
        std::filesystem::current_path(executableRoot, pathError);
#endif
    } else if (pathError) {
        executableRoot.clear();
    }

    if (executableRoot.empty()) {
        std::error_code fallbackError;
        executableRoot = std::filesystem::current_path(fallbackError);
        if (fallbackError) {
            std::cerr << "Warning: Builder could not determine its executable directory." << std::endl;
            executableRoot.clear();
        }
    }
    if (!executableRoot.empty()) setBuilderRootEnvironment(executableRoot);

    int windowWidth = 1024;
    int windowHeight = 640;
    SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow(
        "Bloons TD 4 Repopped - Game Builder", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight, windowFlags);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        std::cerr << "Error creating SDL_Renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    btd4::BuilderUI builderUI;
    builderUI.initialize();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window)) running = false;

            if (event.type == SDL_DROPFILE) {
                if (event.drop.file) {
                    builderUI.addSourceFile(event.drop.file);
                    SDL_free(event.drop.file);
                }
            }
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        builderUI.render();
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 30, 30, 35, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv);
}
#endif
