#include <iostream>
#include <vector>
#include <cmath>
#include <queue>
#include <random>
#include <SDL.h>
#include <SDL_ttf.h>

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

vector<vector<int>> vSandMap(floor(iWindowWidth / iSandSize), vector<int>(floor((iWindowHeight - 60) / iSandSize), 0));

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
                    row.resize(iNewHeight, 0);
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
        vSandMap[x][y] = vSandMap[x][y] != 1 ? 0 : vSandMap[x][y];
    }

    const int iKeyBoxX = floor(iSandSummonX / iSandSize),
              iKeyBoxY = floor((windowSize.h - iSandSummonY) / iSandSize);

    for (int i = static_cast<int>(-1 * ceil(iSandSummonSize / 2)); i <= floor(iSandSummonSize / 2); i++) {
        for (int j = static_cast<int>(-1 * ceil(iSandSummonSize / 2)); j <= floor(iSandSummonSize / 2); j++) {
            if (iKeyBoxX + i >= 0 && iKeyBoxX + i < vSandMap.size() && iKeyBoxY + j >= 0 && iKeyBoxY + j < vSandMap.at(iKeyBoxX + i).size() && vSandMap[iKeyBoxX + i][iKeyBoxY + j] != 1) {
                vSandMap[iKeyBoxX + i][iKeyBoxY + j] = 2;
                qSummonerPosRemove->push({iKeyBoxX + i, iKeyBoxY + j});
            }
        }
    }
}

void drawSand(SDL_Renderer* renderer) {

    const int iKeyBoxX = floor(iSandSummonX / iSandSize),
              iKeyBoxY = floor((windowSize.h - iSandSummonY) / iSandSize);

    for (int i = static_cast<int>(-1 * ceil(iSandSummonSize / 2)); i <= floor(iSandSummonSize / 2); i++) {
        for (int j = static_cast<int>(-1 * ceil(iSandSummonSize / 2)); j <= floor(iSandSummonSize / 2); j++) {
            if (iKeyBoxX + i >= 0 && iKeyBoxX + i < vSandMap.size() && iKeyBoxY + j >= 0 && iKeyBoxY + j < vSandMap.at(iKeyBoxX + i).size() && vSandMap[iKeyBoxX + i][iKeyBoxY + j] != 1) {
                vSandMap[iKeyBoxX + i][iKeyBoxY + j] = 1;
            }
        }
    }
}


void gridDrawer(SDL_Renderer* renderer, const vector<vector<int>>& vSandMap) {
    for (int i = 0; i < vSandMap.size(); i++) {
        for (int j = 0; j < vSandMap[i].size(); j++) {
            const SDL_Rect gridRect{i * iSandSize, windowSize.h - (j + 1) * iSandSize, iSandSize, iSandSize};
            if (vSandMap[i][j] == 1) {
                SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
                SDL_RenderFillRect(renderer, &gridRect);
                SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF);
                SDL_RenderDrawRect(renderer, &gridRect);
            }
            else if (vSandMap[i][j] == 2) {
                SDL_SetRenderDrawColor(renderer, 0x8C, 0x8C, 0x8C, 0xFF);
                SDL_RenderFillRect(renderer, &gridRect);
                SDL_SetRenderDrawColor(renderer, 0x7C, 0x7C, 0x7C, 0xFF);
                SDL_RenderDrawRect(renderer, &gridRect);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 0x3C, 0x3C, 0x3C, 0xFF);
                SDL_RenderDrawRect(renderer, &gridRect);
            }
        }
    }
}


void simulateSandFalling(SDL_Renderer* renderer) {
    if (const Uint64 iCurrentTime = SDL_GetTicks(); iCurrentTime - iLastUpdate > iUpdateInterval) {
        iLastUpdate = iCurrentTime;
        for (int i = 0; i < vSandMap[0].size(); i++) {
            for (int j = 0; j < vSandMap.size(); j++) {
                if (vSandMap[j][i] == 1) {
                    if (i - 1 >= 0) {
                        if (vSandMap[j][i - 1] != 1){
                            vSandMap[j][i] = 0;
                            vSandMap[j][i - 1] = 1;
                        }
                        else {
                            bool isLeftAvailable = false;
                            if (j - 1 < 0) {
                                vSandMap[j][i] = 0;
                            } else if (vSandMap[j - 1][i - 1] != 1) {
                                isLeftAvailable = true;
                            }
                            if (j + 1 >= vSandMap.size()) {
                                vSandMap[j][i] = 0;
                            } else if (vSandMap[j + 1][i - 1] != 1) {
                                if (isLeftAvailable && dis(gen) == 1) {
                                    vSandMap[j][i] = 0;
                                    vSandMap[j - 1][i - 1] = 1;
                                } else {
                                    vSandMap[j][i] = 0;
                                    vSandMap[j + 1][i - 1] = 1;
                                }
                            } else if (isLeftAvailable) {
                                vSandMap[j][i] = 0;
                                vSandMap[j - 1][i - 1] = 1;
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

    if (TTF_Init() == -1) {
        throw SDLException("Couldn't initialize TTF");
    }

    TTF_Font * font = TTF_OpenFont("./TTF/OpenSans.ttf", 10);
    if (font == nullptr) {
        throw SDLException("Couldn't create font.");
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



        string sInformation{};
        // string sInformation{"Sand summoner size: " + to_string(iSandSummonSize) + "\n"};                                           // Size information
        // sInformation += "Frame update interval ( the lower the faster sand fall ): " + to_string(iUpdateInterval) + "ms \n";   // Speed information
        //        sInformation += "Help: Scroll down to decrease sand summoner size, up to increase | Press '+' to increase speed and '-' of sand falling.";

        constexpr SDL_Color White = {0xFF, 0xFF, 0xFF, 0xFF};

        SDL_Surface* surfaceInformation{TTF_RenderText_Solid(font, sInformation.c_str(), White)};

        SDL_Texture* Information = SDL_CreateTextureFromSurface(renderer, surfaceInformation);

        SDL_Rect informationRect{5, 5, iWindowWidth - 10, 55};

        SDL_RenderCopy(renderer, Information, nullptr, &informationRect);

        while (SDL_PollEvent(&event)) {
            switch (event.type){
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        drawSand(renderer);
                        isHoldingLeftMouse = true;
                    } else {
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
                        iSandSummonSize += iSandSummonSize < 10 ? 2 : 0;
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
                            iUpdateInterval -= iUpdateInterval > 25 ? 25 : iUpdateInterval > 5 ? 5 : 0;
                            cout << "Sand Falling Speed: " << iUpdateInterval << endl;
                            break;
                        case SDLK_KP_MINUS:
                            iUpdateInterval += 25;
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
            } else if (isHoldingRightMouse) {
                // TODO: Handle Holding Right Click To Be Able To Remove Sand
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