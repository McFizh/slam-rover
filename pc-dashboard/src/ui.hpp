#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>
#include <functional>

namespace ui
{
    class Window
    {
    public:
        Window(std::string title, SDL_FRect windowBounds);
        ~Window();

        using ContentDrawer = std::function<void(SDL_Renderer *, SDL_FRect content)>;
        void SetContentDrawer(ContentDrawer drawer);

        SDL_FRect GetContentBounds();

        void Draw(SDL_Renderer *renderer, TTF_Font *font);

    private:
        std::string _title;
        ContentDrawer _contentDrawer;
        SDL_FRect _windowBounds;

        bool _titleTextureReady = false;
    };
}