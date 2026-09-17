#define SDL_MAIN_HANDLED
#include "ui/BuilderUI.hpp"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <filesystem>
#include <cstdlib>

namespace {
void setBuilderRootEnvironment(const std::filesystem::path& root) {
#ifdef _WIN32
    _putenv_s("BTD4_BUILDER_ROOT=" + root.string());
#else
    setenv("BTD4_BUILDER_ROOT", root.string().c_str(), 1);
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

    std::filesystem::path executableRoot;
    if (char* basePath = SDL_GetBasePath()) {
        std::error_code pathError;
        executableRoot = std::filesystem::path(basePath).lexically_normal();
        std::filesystem::current_path(executableRoot, pathError);
        SDL_free(basePath);
        if (pathError) {
            std::cerr << "Warning: Could not set builder working directory: " << pathError.message() << std::endl;
        }
    }
    if (executableRoot.empty()) executableRoot = std::filesystem::current_path();
    setBuilderRootEnvironment(executableRoot);

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
