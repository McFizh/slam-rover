#include <string>

#include <SDL3_image/SDL_image.h>

#include "controller.hpp"
#include "network/network.hpp"
#include "ui.hpp"

SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *robotoFont;

void shutdownSDL() {
  if (renderer != NULL)
    SDL_DestroyRenderer(renderer);

  if (window != NULL)
    SDL_DestroyWindow(window);

  if (robotoFont != NULL)
    TTF_CloseFont(robotoFont);

  TTF_Quit();
  SDL_Quit();
}

/**
 * Initialize SDL library, create window + renderer
 */
int initSDL() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    SDL_Log("SDL_Init failed with error: %s", SDL_GetError());
    return 1;
  }

  if (!TTF_Init()) {
    SDL_Log("TTF_Init failed with error: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  window = SDL_CreateWindow("Dashboard", 1200, 800, SDL_WINDOW_RESIZABLE);
  if (!window) {
    SDL_Log("SDL_CreateWindow failed with error: %s", SDL_GetError());
    shutdownSDL();
    return 1;
  }

  renderer = SDL_CreateRenderer(window, nullptr);
  if (!renderer) {
    SDL_Log("SDL_CreateRenderer failed with error: %s", SDL_GetError());
    shutdownSDL();
    return 1;
  }

  const char *base_path = SDL_GetBasePath();
  const std::string font_path = std::string(base_path ? base_path : "./") + "assets/Roboto-Medium.ttf";
  robotoFont = TTF_OpenFont(font_path.c_str(), 16);
  if (!robotoFont) {
    SDL_Log("TTF_OpenFont failed for file '%s' with error: %s", font_path.c_str(), SDL_GetError());
    shutdownSDL();
    return 1;
  }

  return 0;
}

bool pollEvents(input::ControllerHandler &controllers, network::Client &commandClient) {
  SDL_Event event;
  bool runMainloop = true;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
      runMainloop = false;
    } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
      // Connect and disconnect keybindings
      if (event.key.key == SDLK_C) {
        if (commandClient.GetConnectionState() == network::ConnectionState::Disconnected) {
          commandClient.Connect();
        }
      } else if (event.key.key == SDLK_D) {
        if (commandClient.GetConnectionState() != network::ConnectionState::Disconnected) {
          commandClient.Disconnect();
        }
      }
    }

    controllers.HandleEvent(event);
  }

  return runMainloop;
}

/**
 * Run main loop in it's own method, so that windows and controllers get deleted before SDL cleanup is done
 */
void runMainLoop(network::Server &telemetryServer) {
  ui::Window camera_window("Camera feed", SDL_FRect{20, 40, 640, 480});
  input::ControllerHandler controllers;
  network::Client commandClient;

  SDL_Texture *cameraTexture = NULL;
  std::vector<uint8_t> jpegData;

  camera_window.SetContentDrawer([&cameraTexture](SDL_Renderer *renderer, SDL_FRect content) {
    if (!cameraTexture)
      return;

    float txtW, txtH;
    SDL_GetTextureSize(cameraTexture, &txtW, &txtH);
    SDL_FRect dstRect{content.x, content.y, txtW, txtH};
    SDL_RenderTexture(renderer, cameraTexture, NULL, &dstRect);
  });

  bool loopRunning = true;
  while (loopRunning) {
    loopRunning = pollEvents(controllers, commandClient);

    if (telemetryServer.GetLatestFrame(jpegData)) {
      SDL_IOStream *io = SDL_IOFromMem(jpegData.data(), jpegData.size());
      SDL_Surface *surface = IMG_Load_IO(io, true);
      if (surface) {
        SDL_DestroyTexture(cameraTexture);
        cameraTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    camera_window.Draw(renderer, robotoFont);

    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  SDL_DestroyTexture(cameraTexture);
}

int main() {
  if (initSDL()) {
    return 1;
  }

  network::Server telemetryServer;

  if (telemetryServer.Start() == 0) {
    runMainLoop(telemetryServer);
    telemetryServer.Stop();
  } else {
    SDL_Log("Failed to start UDP telemetry server");
  }

  // Cleanup and quit
  shutdownSDL();
  return 0;
}