#include "application.h"
#include <iostream>
#include <SDL_vulkan.h>

Application::Application(const char *appName) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        throw std::runtime_error("Failed to initialize SDL");
    }

    if (SDL_Vulkan_LoadLibrary(nullptr) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        throw std::runtime_error("Failed to initialize SDL");
    }

    window = SDL_CreateWindow(appName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 760,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    vulkan.initialize(appName, window);
}

Application::~Application() {
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
}

void Application::start() {
    running = true;

    while (running) {
        running = processEvents();
        vulkan.update();
        vulkan.renderFrame();
    }
}

bool Application::processEvents() {
    SDL_Event event;
    bool shouldContinueRunning = true;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                shouldContinueRunning &= false;
                break;
            case SDL_KEYDOWN:
                shouldContinueRunning &= handleKeyboardEvent(event.key);
                break;
            case SDL_MOUSEBUTTONDOWN:
                break;
            case SDL_MOUSEMOTION:
                break;
            default:
                break;
        }
    }

    return shouldContinueRunning;
}

bool Application::handleKeyboardEvent(const SDL_KeyboardEvent &event) {
    switch (event.keysym.sym) {
        case SDLK_ESCAPE:
            return false;
        default:
            return true;
    }
}
