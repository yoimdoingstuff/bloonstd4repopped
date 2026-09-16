#define SDL_MAIN_HANDLED
#include "../engine/core/Engine.hpp"
#include "../engine/core/Logger.hpp"
#include "../engine/input/FrontendProfile.hpp"
#include "../platform/common/SDLRenderer.hpp"
#include "../platform/common/SDLInput.hpp"
#include <SDL.h>
#include <filesystem>
#include <iostream>

namespace {
std::string executableDirectory() {
    char* base = SDL_GetBasePath();
    if (!base) return ".";
    std::string result(base);
    SDL_free(base);
    while (!result.empty() && (result.back() == '/' || result.back() == '\\')) result.pop_back();
    return result.empty() ? "." : result;
}
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    BTD4_LOG_INFO("Starting Bloons TD 4 Repopped...");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Packaged builds keep game_data beside the executable. Use the executable
    // directory as the working directory so double-clicking the game works too.
    const std::string baseDir = executableDirectory();
    std::error_code ec;
    std::filesystem::current_path(baseDir, ec);
    if (ec) BTD4_LOG_WARN("Could not switch to executable directory: " + ec.message());

    int windowWidth = 960;
    int windowHeight = 544;
    SDL_Window* window = SDL_CreateWindow(
        "Bloons TD 4 Repopped",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
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

    btd4::SDLInput input(btd4::FrontendProfile::FlashDesktop);
    btd4::Engine engine(renderer, input, btd4::FrontendProfile::FlashDesktop);

    if (!engine.initialize(windowWidth, windowHeight)) {
        std::cerr << "Failed to initialize engine" << std::endl;
        renderer.shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;
    while (running && engine.isRunning()) {
        input.beginFrame();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            else if (event.type == SDL_WINDOWEVENT &&
                     (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                      event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
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
