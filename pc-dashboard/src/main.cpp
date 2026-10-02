#include <string>

#include "ui.hpp"
#include "controller.hpp"
#include "network.hpp"

SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *robotoFont;

void shutdownSDL()
{
    if (renderer != NULL)
        SDL_DestroyRenderer(renderer);

    if (window != NULL)
        SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}

/**
 * Initialize SDL library, create window + renderer
 */
int initSDL()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        SDL_Log("SDL_Init failed with error: %s", SDL_GetError());
        return 1;
    }

    if (!TTF_Init())
    {
        SDL_Log("TTF_Init failed with error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("Dashboard", 1000, 700, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("SDL_CreateWindow failed with error: %s", SDL_GetError());
        shutdownSDL();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        SDL_Log("SDL_CreateRenderer failed with error: %s", SDL_GetError());
        shutdownSDL();
        return 1;
    }

    const char *base_path = SDL_GetBasePath();
    const std::string font_path = std::string(base_path ? base_path : "./") + "assets/Roboto-Medium.ttf";
    robotoFont = TTF_OpenFont(font_path.c_str(), 16);
    if (!robotoFont)
    {
        SDL_Log("TTF_OpenFont failed for file '%s' with error: %s", font_path.c_str(), SDL_GetError());
        shutdownSDL();
        return 1;
    }

    return 0;
}

bool pollEvents(input::ControllerHandler &controllers)
{
    SDL_Event event;
    bool runMainloop = true;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
        {
            runMainloop = false;
        }

        controllers.HandleEvent(event);
    }

    return runMainloop;
}

int main()
{
    if (initSDL())
    {
        return 1;
    }

    ui::Window test_window("Test", SDL_FRect{40, 40, 300, 180});
    input::ControllerHandler controllers;

    bool runMainloop = true;
    while (runMainloop)
    {
        runMainloop = pollEvents(controllers);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        test_window.Draw(renderer, robotoFont);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Cleanup and quit
    shutdownSDL();
    return 0;
}