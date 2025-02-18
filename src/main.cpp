#include <iostream>
#include <vector>
#include <cmath>
#include <queue>
#include <random>
#include <SDL.h>

#define HuyN_ int main
#define iWindowWidth 1280
#define iWindowHeight 720

using std::string, std::cout, std::cerr, std::endl, std::vector, std::floor, std::ceil, std::abs, std::queue, std::rand, std::to_string;

class SDLException final : public std::runtime_error {
public:
    explicit SDLException(const std::string& message) : std::runtime_error(message + "\n" + SDL_GetError()) {
        cerr << "SDL Error: " << message << endl;
    }
};

// Global random number generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(0, 1);

// Global Variables Area

int iSandSize = 12;
int iSandSummonSize = 3;
int iSandSummonX = 0, iSandSummonY = 0;

bool isHoldingLeftMouse = false, isHoldingRightMouse = false;

Uint64 iLastUpdate = 0; // Last Update Time
Uint64 iUpdateInterval = 30; // Frame update interval

struct Pos {
    int x,
        y;
};

struct {
    int w = iWindowWidth;
    int h = iWindowHeight;
} windowSize;

struct SandProperties {
    int iSandType = 0;
    bool isHover = false;
};

vector<vector<SandProperties>> vSandMap(floor(iWindowWidth / iSandSize), vector<SandProperties>(floor((iWindowHeight - 60) / iSandSize)));

/*
Sand Identification
    #id     #Name           #Reason
    0       Empty           Empty box
    1       Falling-Sand    Sand is in midair
    2       Sand-Summoner   Following cursor
*/


// Functions Area


static int resizingEventWatcher(void* data, const SDL_Event* event) {
    if (event->type == SDL_WINDOWEVENT &&
        event->window.event == SDL_WINDOWEVENT_RESIZED) {
            if (const SDL_Window* win = SDL_GetWindowFromID(event->window.windowID); win == static_cast<SDL_Window *>(data)) {
                windowSize.w = event->window.data1;
                windowSize.h = event->window.data2;
                const int iNewWidth = floor(windowSize.w / iSandSize);
                const int iNewHeight = floor((windowSize.h - 60) / iSandSize);

                vSandMap.resize(iNewWidth);
                for (auto& row : vSandMap) {
                    row.resize(iNewHeight, {0, false});
                }
            }
        }
    return 0;
}

void setSandSummoner(SDL_Renderer* renderer, queue<Pos> *qSummonerPosRemove) {
    // Old summoner Removal
    while (!qSummonerPosRemove->empty()) {
        const auto [x, y] = qSummonerPosRemove->front();
        qSummonerPosRemove->pop();
        vSandMap[x][y].isHover = false;
    }

    const int iKeyBoxX = floor(iSandSummonX / iSandSize),
              iKeyBoxY = floor((windowSize.h - iSandSummonY) / iSandSize);

    for (int i = static_cast<int>(-1 * ceil(iSandSummonSize / 2)); i <= floor(iSandSummonSize / 2); i++) {
        for (int j = static_cast<int>(-1 * ceil(iSandSummonSize / 2)); j <= floor(iSandSummonSize / 2); j++) {
            if (iKeyBoxX + i >= 0 && iKeyBoxX + i < vSandMap.size() && iKeyBoxY + j >= 0 && iKeyBoxY + j < vSandMap.at(iKeyBoxX + i).size()) {
                vSandMap[iKeyBoxX + i][iKeyBoxY + j].isHover = true;
                qSummonerPosRemove->push({iKeyBoxX + i, iKeyBoxY + j});
                if (isHoldingRightMouse) vSandMap[iKeyBoxX + i][iKeyBoxY + j].iSandType = 0;
            }
        }
    }
}

void drawSand(SDL_Renderer* renderer) {

    const int iKeyBoxX = floor(iSandSummonX / iSandSize),
              iKeyBoxY = floor((windowSize.h - iSandSummonY) / iSandSize);

    for (int i = static_cast<int>(-1 * ceil(iSandSummonSize / 2)); i <= floor(iSandSummonSize / 2); i++) {
        for (int j = static_cast<int>(-1 * ceil(iSandSummonSize / 2)); j <= floor(iSandSummonSize / 2); j++) {
            if (iKeyBoxX + i >= 0 && iKeyBoxX + i < vSandMap.size() && iKeyBoxY + j >= 0 && iKeyBoxY + j < vSandMap.at(iKeyBoxX + i).size() && vSandMap[iKeyBoxX + i][iKeyBoxY + j].iSandType != 1) {
                vSandMap[iKeyBoxX + i][iKeyBoxY + j].iSandType = 1;
            }
        }
    }
}
void eraseSand(SDL_Renderer* renderer) {

    const int iKeyBoxX = floor(iSandSummonX / iSandSize),
              iKeyBoxY = floor((windowSize.h - iSandSummonY) / iSandSize);

    for (int i = static_cast<int>(-1 * ceil(iSandSummonSize / 2)); i <= floor(iSandSummonSize / 2); i++) {
        for (int j = static_cast<int>(-1 * ceil(iSandSummonSize / 2)); j <= floor(iSandSummonSize / 2); j++) {
            if (iKeyBoxX + i >= 0 && iKeyBoxX + i < vSandMap.size() && iKeyBoxY + j >= 0 && iKeyBoxY + j < vSandMap.at(iKeyBoxX + i).size() && vSandMap[iKeyBoxX + i][iKeyBoxY + j].iSandType == 1) {
                vSandMap[iKeyBoxX + i][iKeyBoxY + j].iSandType = 0;
            }
        }
    }
}


void gridDrawer(SDL_Renderer* renderer, const vector<vector<SandProperties>>& vSandMap) {
    for (int i = 0; i < vSandMap.size(); i++) {
        for (int j = 0; j < vSandMap[i].size(); j++) {
            const SDL_Rect gridRect{i * iSandSize, windowSize.h - (j + 1) * iSandSize, iSandSize, iSandSize};

            if (vSandMap[i][j].iSandType == 1) {
                if (vSandMap[i][j].isHover == true) {
                    SDL_SetRenderDrawColor(renderer, 0xFF, 0xF3, 0xA1, 0xFF);
                    SDL_RenderFillRect(renderer, &gridRect);
                    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
                    SDL_RenderDrawRect(renderer, &gridRect);
                } else {
                    SDL_SetRenderDrawColor(renderer, 0xFF, 0xF3, 0xA1, 0xFF);
                    SDL_RenderFillRect(renderer, &gridRect);
                    SDL_SetRenderDrawColor(renderer, 0xFF, 0xE8, 0x66, 0xFF);
                    SDL_RenderDrawRect(renderer, &gridRect);
                }
            }
            else {
                if (vSandMap[i][j].isHover == true) {
                    SDL_SetRenderDrawColor(renderer, 0x8C, 0x8C, 0x8C, 0xFF);
                    SDL_RenderFillRect(renderer, &gridRect);
                    SDL_SetRenderDrawColor(renderer, 0x7C, 0x7C, 0x7C, 0xFF);
                    SDL_RenderDrawRect(renderer, &gridRect);
                } else {
                    SDL_SetRenderDrawColor(renderer, 0x3C, 0x3C, 0x3C, 0xFF);
                    SDL_RenderDrawRect(renderer, &gridRect);
                }
            }
        }
    }
}


void simulateSandFalling(SDL_Renderer* renderer) {
    if (const Uint64 iCurrentTime = SDL_GetTicks(); iCurrentTime - iLastUpdate > iUpdateInterval && !isHoldingRightMouse) {
        iLastUpdate = iCurrentTime;
        for (int i = 0; i < vSandMap[0].size(); i++) {
            for (int j = 0; j < vSandMap.size(); j++) {
                if (vSandMap[j][i].iSandType == 1) {
                    if (i - 1 >= 0) {
                        if (vSandMap[j][i - 1].iSandType != 1){
                            vSandMap[j][i].iSandType = 0;
                            vSandMap[j][i - 1].iSandType = 1;
                        }
                        else {
                            bool isLeftAvailable = false;
                            if (j - 1 >= 0 && vSandMap[j - 1][i - 1].iSandType != 1) {
                                isLeftAvailable = true;
                            }
                            if (j + 1 < vSandMap.size() && vSandMap[j + 1][i - 1].iSandType != 1) {
                                if (isLeftAvailable && dis(gen) == 1) {
                                    vSandMap[j][i].iSandType = 0;
                                    vSandMap[j - 1][i - 1].iSandType = 1;
                                } else {
                                    vSandMap[j][i].iSandType = 0;
                                    vSandMap[j + 1][i - 1].iSandType = 1;
                                }
                            } else if (isLeftAvailable) {
                                vSandMap[j][i].iSandType = 0;
                                vSandMap[j - 1][i - 1].iSandType = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    gridDrawer(renderer, vSandMap);
}

HuyN_(int argc, char* argv[]) {

    queue<Pos> qSummonerPosRemove = {};

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


    const string sGuild{"Guild: Scroll down to decrease sand summoner size, up to increase | Press '+' to increase speed and '-' of sand falling. \nThis Simulator Was Created by HuyN... Appreciate my works pls..."};


    constexpr SDL_MessageBoxButtonData messageBoxButton{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Understood."};

    const SDL_MessageBoxData messageBoxData{SDL_MESSAGEBOX_INFORMATION, window, "Guild to use the simulator.", sGuild.c_str(), 1, &messageBoxButton, nullptr};

    SDL_ShowMessageBox(&messageBoxData, nullptr);


    while (isRunning) {

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        while (SDL_PollEvent(&event)) {
            switch (event.type){
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        drawSand(renderer);
                        isHoldingLeftMouse = true;
                    } else {
                        eraseSand(renderer);
                        isHoldingRightMouse = true;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        isHoldingLeftMouse = false;
                    } else {
                        isHoldingRightMouse = false;
                    }
                    break;
                case SDL_MOUSEWHEEL:
                    if (event.wheel.y > 0) {
                        iSandSummonSize += iSandSummonSize < 19 ? 2 : 0;
                    } else if (event.wheel.y < 0) {
                        iSandSummonSize -= iSandSummonSize > 1 ? 2 : 0;
                    }
                    cout << "Sand Summoner Size: " << iSandSummonSize << endl;
                    setSandSummoner(renderer, &qSummonerPosRemove);
                    break;
                case SDL_MOUSEMOTION:
                    iSandSummonX = event.motion.x;
                    iSandSummonY = event.motion.y;
                    setSandSummoner(renderer, &qSummonerPosRemove);
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_KP_PLUS:
                            iUpdateInterval -= iUpdateInterval > 15 ? 15 : iUpdateInterval > 5 ? 5 : 0;
                            cout << "Sand Falling Speed: " << iUpdateInterval << endl;
                            break;
                        case SDLK_KP_MINUS:
                            iUpdateInterval += 15;
                            cout << "Sand Falling Speed: " << iUpdateInterval << endl;
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_QUIT:
                    isRunning = false;
                    break;
                default:
                    break;
            }
            if (isHoldingLeftMouse) {
                drawSand(renderer);
            }
        }

        simulateSandFalling(renderer);

        SDL_RenderPresent(renderer);
        //
        // SDL_FreeSurface(surfaceInformation);
        // SDL_DestroyTexture(Information);

    }

    return EXIT_SUCCESS;
}