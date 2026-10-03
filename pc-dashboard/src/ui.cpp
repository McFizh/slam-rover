#include "ui.hpp"

namespace ui {
  constexpr float TITLE_BAR_HEIGHT = 26.0f;
  constexpr float TITLE_TEXT_PADDING = 8.0f;

  constexpr SDL_Color WINDOW_COLOR{35, 35, 40, 235};
  constexpr SDL_Color TITLE_BAR_COLOR{60, 90, 140, 255};
  constexpr SDL_Color TITLE_TEXT_COLOR{255, 255, 255, 255};
  constexpr SDL_Color BORDER_COLOR{200, 200, 200, 255};

  Window::Window(std::string title, SDL_FRect windowBounds) : _title(std::move(title)), _windowBounds(windowBounds) {}

  Window::~Window() { SDL_DestroyTexture(_titleTexture); }

  void Window::SetContentDrawer(ContentDrawer drawer) { _contentDrawer = drawer; }

  SDL_FRect Window::GetContentBounds() {
    return {_windowBounds.x, _windowBounds.y + TITLE_BAR_HEIGHT, _windowBounds.w, _windowBounds.h - TITLE_BAR_HEIGHT};
  }

  void Window::CreateTitleTexture(SDL_Renderer *renderer, const SDL_FRect *titleBarBounds, TTF_Font *font) {
    SDL_Surface *surface = TTF_RenderText_Blended(font, _title.c_str(), 0, TITLE_TEXT_COLOR);
    if (surface) {
      _titleTexture = SDL_CreateTextureFromSurface(renderer, surface);
      SDL_DestroySurface(surface);

      // Calculate source rectangle
      float txtW = 0.0f, txtH = 0.0f, maxW = 0.0f;
      maxW = SDL_max(titleBarBounds->w - 2.0f * TITLE_TEXT_PADDING, 0.0f);
      SDL_GetTextureSize(_titleTexture, &txtW, &txtH);
      _titleSrcRect.w = SDL_min(txtW, maxW);
      _titleSrcRect.h = txtH;

    } else {
      SDL_Log("TTF_RenderText_Blended failed with error: %s", SDL_GetError());
    }
  }

  void Window::Draw(SDL_Renderer *renderer, TTF_Font *font) {
    const SDL_FRect contentBounds = GetContentBounds();

    // Clear the window area
    SDL_SetRenderDrawColor(renderer, WINDOW_COLOR.r, WINDOW_COLOR.g, WINDOW_COLOR.b, WINDOW_COLOR.a);
    SDL_RenderFillRect(renderer, &contentBounds);

    // Draw title bar box
    const SDL_FRect titleBarBounds{_windowBounds.x, _windowBounds.y, _windowBounds.w, TITLE_BAR_HEIGHT};
    SDL_SetRenderDrawColor(renderer, TITLE_BAR_COLOR.r, TITLE_BAR_COLOR.g, TITLE_BAR_COLOR.b, TITLE_BAR_COLOR.a);
    SDL_RenderFillRect(renderer, &titleBarBounds);

    // Create the title text, if it doesn't exist
    if (!_titleTexture) {
      CreateTitleTexture(renderer, &titleBarBounds, font);
    }

    // Draw the title texture
    if (_titleTexture) {
      const SDL_FRect dstRect{titleBarBounds.x + TITLE_TEXT_PADDING,
                              SDL_floorf(titleBarBounds.y + (TITLE_BAR_HEIGHT - _titleSrcRect.h) / 2.0f) + 1,
                              _titleSrcRect.w, _titleSrcRect.h};

      SDL_RenderTexture(renderer, _titleTexture, &_titleSrcRect, &dstRect);
    }

    // Draw box around the window
    SDL_SetRenderDrawColor(renderer, BORDER_COLOR.r, BORDER_COLOR.g, BORDER_COLOR.b, BORDER_COLOR.a);
    SDL_RenderRect(renderer, &_windowBounds);

    // Run external content drawer method (if one is set)
    if (_contentDrawer) {
      _contentDrawer(renderer, contentBounds);
    }
  }
} // namespace ui