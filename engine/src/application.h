#pragma once

#include <SDL.h>
#include "renderer/vulkan.h"

class Application {
public:
    explicit Application(const char *appName);
    ~Application();

    void start();

protected:
private:
    SDL_Window *window = nullptr;
    Vulkan vulkan;
    bool running = false;

    bool processEvents();
    bool handleKeyboardEvent(const SDL_KeyboardEvent &event);
};