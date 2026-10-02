#pragma once

#include <SDL3/SDL.h>

#include <unordered_map>

namespace input
{
    class ControllerHandler
    {
    public:
        ControllerHandler() = default;
        ~ControllerHandler();

        void HandleEvent(const SDL_Event &event);

    private:
        struct Controller
        {
            SDL_Gamepad *gamepad = NULL;
        };

        std::unordered_map<SDL_JoystickID, Controller> _controllers;
        void ProcessGamepadEvent(const SDL_JoystickID id);
    };
}