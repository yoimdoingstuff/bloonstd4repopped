#include "../engine/core/Engine.hpp"
#include "../engine/core/Logger.hpp"
#include "../engine/input/FrontendProfile.hpp"
#include "../platform/common/SDLRenderer.hpp"
#include "../platform/common/SDLInput.hpp"
#include "../platform/common/SDLAudio.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::string executableDirectory() {
    char* base = SDL_GetBasePath();
    if (!base) return ".";
    std::string result(base);
    SDL_free(base);
    while (!result.empty() && (result.back() == '/' || result.back() == '\\')) result.pop_back();
    return result.empty() ? "." : result;
}

std::string findImportedDataDirectory() {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path root("game_data");
    if (fs::exists(root / "manifest.json", ec)) return root.string();
    if (!fs::is_directory(root, ec)) return {};
    std::vector<fs::path> manifests;
    for (const auto& entry : fs::directory_iterator(root, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) break;
        if (!entry.is_directory(ec)) continue;
        if (fs::is_regular_file(entry.path() / "manifest.json", ec)) manifests.push_back(entry.path());
    }
    if (manifests.size() == 1) return manifests.front().string();
    return {};
}
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    BTD4_LOG_INFO("Starting Bloons TD 4 Repopped...");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    const std::string baseDir = executableDirectory();
    std::error_code ec;
    std::filesystem::current_path(baseDir, ec);
    if (ec) BTD4_LOG_WARN("Could not switch to executable directory: " + ec.message());

    int windowWidth = 960;
    int windowHeight = 544;
    SDL_Window* window = SDL_CreateWindow(
        "Bloons TD 4 Repopped",
        windowWidth,
        windowHeight,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    btd4::SDLRenderer renderer;
    if (!renderer.initializeWithWindow(window)) {
        std::cerr << "Failed to initialize SDL renderer" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Desktop builds always use the Flash-style mouse-first frontend. PSP input
    // is selected only by the PSP entry point, so importing a PSP source asset
    // set cannot accidentally change Windows controls.
    btd4::SDLInput input(btd4::FrontendProfile::FlashDesktop);
    btd4::SDLAudio audio;
    btd4::Engine engine(renderer, input, btd4::FrontendProfile::FlashDesktop, &audio);

    if (!engine.initialize(windowWidth, windowHeight)) {
        std::cerr << "Failed to initialize engine" << std::endl;
        renderer.shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const std::string importedData = findImportedDataDirectory();
    if (!importedData.empty()) {
        BTD4_LOG_INFO("Imported game data detected at: " + importedData);
    } else {
        BTD4_LOG_INFO("No unique imported edition detected; runtime fallback data remains available.");
    }

    bool running = true;
    SDL_Event event;
    while (running && engine.isRunning()) {
        input.beginFrame();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                windowWidth = event.window.data1;
                windowHeight = event.window.data2;
                engine.onResize(windowWidth, windowHeight);
            }
            input.processEvent(event, engine.currentViewport());
        }
        engine.frame(windowWidth, windowHeight);
    }

    engine.shutdown();
    renderer.shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    BTD4_LOG_INFO("Bloons TD 4 Repopped exited cleanly.");
    return 0;
}
