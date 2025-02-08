#include <iostream>
#include <vector>
#include <cmath>
#include <SDL.h>

#define HuyN_ int main
#define iWindowWidth 1280
#define iWindowHeight 720

using std::cout, std::cerr, std::endl, std::vector, std::floor, std::ceil, std::abs;

class SDLException final : public std::runtime_error {
public:
    explicit SDLException(const std::string& message) : std::runtime_error(message + "\n" + SDL_GetError()) {
        cerr << "SDL Error: " << message << endl;
    }
};

int iSandSize = 5;
int iSandSummonSize = 20;
int iSandSummonX = 0, iSandSummonY = 0;

int iUpdateSpeed = 50; // 50ms

struct {
    int w = iWindowWidth;
    int h = iWindowHeight;
} windowSize;

static int resizingEventWatcher(void* data, const SDL_Event* event) {
    if (event->type == SDL_WINDOWEVENT &&
        event->window.event == SDL_WINDOWEVENT_RESIZED) {
            if (const SDL_Window* win = SDL_GetWindowFromID(event->window.windowID); win == static_cast<SDL_Window *>(data)) {
                windowSize.w = event->window.data1;
                windowSize.h = event->window.data2;
            }
        }
    return 0;
}

void drawSandSummoner(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0x2C, 0x2C, 0x2C, 0xFF);
    const SDL_Rect sandSummonRect{iSandSummonX - iSandSummonSize / 2, iSandSummonY - iSandSummonSize / 2, iSandSummonSize, iSandSummonSize};
    SDL_RenderFillRect(renderer, &sandSummonRect);
}

void drawSand(SDL_Renderer* renderer, const int x, const int y, const int oldX, const int oldY) {
    // Clear Old Sand At Pos (oldX, oldY)
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    const SDL_Rect sandClearRect{oldX, oldY, iSandSize, iSandSize};
    SDL_RenderFillRect(renderer, &sandClearRect);

    // Draw Sand At Pos (x, y)
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    const SDL_Rect sandSummonRect{x, y, iSandSize, iSandSize};
    SDL_RenderFillRect(renderer, &sandSummonRect);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF);
    SDL_RenderDrawRect(renderer, &sandSummonRect);
}


void gridDrawer(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    for (int x = 0; x < windowSize.w; x += iSandSize) {
        SDL_RenderDrawLine(renderer, x, 0, x, windowSize.h);
    }
    for (int y = 0; y < windowSize.h; y += iSandSize) {
        SDL_RenderDrawLine(renderer, 0, y, windowSize.w, y);
    }
}


void simulateSandFalling(SDL_Renderer* renderer) {

}

HuyN_(int argc, char* argv[]) {

    vector<vector<int>> sandMap(floor(iWindowWidth / iSandSize), vector<int>(floor(iWindowHeight / iSandSize), 0));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw SDLException("Failed to initialize SDL");
    }

    SDL_Window *window{
        SDL_CreateWindow("Sand Falling Simulator | HuyN", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, iWindowWidth,
                         iWindowHeight, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE)
    };
    if (window == nullptr) {
        throw SDLException("Failed to create window");
    }

    SDL_Renderer *renderer{
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
    };
    if (renderer == nullptr) {
        throw SDLException("Failed to create renderer");
    }

    SDL_ShowWindow(window);

    SDL_AddEventWatch(reinterpret_cast<SDL_EventFilter>(resizingEventWatcher), window);

    SDL_Event event;
    bool isRunning{true};

    while (isRunning) {

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        while (SDL_PollEvent(&event)) {
            switch (event.type){
                case SDL_QUIT:
                    isRunning = false;
                    break;
                case SDL_MOUSEWHEEL:
                    if (event.wheel.y > 0) {
                        iSandSummonSize += iSandSummonSize < 100 ? 10 : 0;
                    } else if (event.wheel.y < 0) {
                        iSandSummonSize -= iSandSummonSize > 10 ? 10 : 0;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    iSandSummonX = event.motion.x;
                    iSandSummonY = event.motion.y;
                    break;
                default:
                    break;
            }
        }

        drawSandSummoner(renderer);

        SDL_RenderPresent(renderer);
    }

    return EXIT_SUCCESS;
}