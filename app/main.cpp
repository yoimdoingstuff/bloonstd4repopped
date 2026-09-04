#define SDL_MAIN_HANDLED
#include "../engine/core/Engine.hpp"
#include "../engine/core/Logger.hpp"
#include "../platform/common/SDLRenderer.hpp"
#include "../platform/common/SDLInput.hpp"
#include <SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    BTD4_LOG_INFO("Starting Bloons TD 4 Repopped...");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Default window size: 960x544 (2x logical 480x272)
    int windowWidth = 960;
    int windowHeight = 544;

    SDL_Window* window = SDL_CreateWindow(
        "Bloons TD 4 Repopped (Test)",
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

    btd4::SDLInput input;
    btd4::Engine engine(renderer, input);

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
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    windowWidth = event.window.data1;
                    windowHeight = event.window.data2;
                    engine.onResize(windowWidth, windowHeight);
                }
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
