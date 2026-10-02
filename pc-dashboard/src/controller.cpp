#include "controller.hpp"

namespace input
{
    /*
    Close all controllers
    */
    ControllerHandler::~ControllerHandler()
    {
        for (auto &[id, controller] : _controllers)
        {
            SDL_CloseGamepad(controller.gamepad);
        }
    }

    /*
    Handle connect/disconnect events, button up/down events and motion events
    */
    void ControllerHandler::HandleEvent(const SDL_Event &event)
    {
        switch (event.type)
        {
        case SDL_EVENT_GAMEPAD_ADDED:
        {
            const SDL_JoystickID id = event.gdevice.which;
            SDL_Gamepad *gamepad = SDL_OpenGamepad(id);
            if (!gamepad)
            {
                SDL_Log("Failed to open gamepad %d due to error: %s", id, SDL_GetError());
                return;
            }
            SDL_Log("Gamepad connected (id: %d)", id);
            _controllers[id] = Controller{gamepad};
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED:
        {
            const SDL_JoystickID id = event.gdevice.which;
            auto iterator = _controllers.find(id);
            if (iterator != _controllers.end())
            {
                SDL_Log("Gamepad disconnected (id: %d)", id);
                SDL_CloseGamepad(iterator->second.gamepad);
                _controllers.erase(iterator);
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            ProcessGamepadEvent(event.gbutton.which);
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            ProcessGamepadEvent(event.gaxis.which);
            break;
        default:
            break;
        }
    }

    /*
    Handle button and motion events for known controllers
    */
    void ControllerHandler::ProcessGamepadEvent(const SDL_JoystickID id)
    {
        // If iterator == _controller.end() => gamepad is not registered for some reason
        auto iterator = _controllers.find(id);
        if (iterator == _controllers.end())
        {
            return;
        }
    }
}