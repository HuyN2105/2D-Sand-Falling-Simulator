#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <queue>
#include <random>
#include <SDL.h>

#define HuyN_ int main
#define iWindowWidth 1280
#define iWindowHeight 720

using std::string, std::cout, std::cerr, std::endl, std::vector, std::floor, std::ceil, std::abs, std::queue, std::rand, std::to_string, std::unique_ptr;

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

int iSandSize = 8;
int iSandSummonSize = 3;
int iSandSummonX = 0, iSandSummonY = 0;

bool isHoldingLeftMouse = false, isHoldingRightMouse = false;

Uint64 iLastUpdate = 0; // Last Update Time
Uint64 iUpdateInterval = 15; // Frame update interval

struct Pos {
    int x,
        y;
};

struct {
    int w = iWindowWidth;
    int h = iWindowHeight;
} windowSize;

class PVector {
public:
    double x;
    double y;

    PVector() : x(0), y(0) {};

    PVector(const double x_, const double y_) : x(x_), y(y_) {};

    void add(const PVector v) {
        x = x + v.x;
        y = y + v.y;
    }

    void distribute(const PVector v) {
        x = x - v.x;
        y = y - v.y;
    }

    [[nodiscard]] PVector rotate(const double deg) const {
        return PVector({x + deg, y + deg});
    }

    [[nodiscard]] PVector mult(const double lhs) const {
        return PVector({x * lhs, y * lhs});
    }

    [[nodiscard]] PVector div(const double lhs) const {
        return PVector({x / lhs, y / lhs});
    }

    [[nodiscard]] double mag() const {
        return sqrt(pow(x, 2) + pow(y, 2));
    }

    [[nodiscard]] PVector normalize() const {
        return div(mag() == 0 ? 1 : mag());
    }

    [[nodiscard]] PVector plus(const PVector v) const {
        return PVector{x + v.x, y + v.y};
    }

    [[nodiscard]] PVector sub(const PVector v) const {
        return PVector{x - v.x, y - v.y};
    }

    [[nodiscard]] double dot(const PVector v) const {
        return x * v.x + y * v.y;
    }

    [[nodiscard]] double dist(const PVector v) const {
        return sqrt(pow(x - v.x, 2) + pow(y - v.y, 2));
    }

    [[nodiscard]] double angleBetween(const PVector v) const {
        return acos(this->dot(v) / (this->mag() * v.mag()));
    }

    void random2D(const int start = 50, const int end = 100) {
        x = rand() % (end - start) + start;
        y = rand() % (end - start) + start;
    }

};

PVector operator*(const double lhs, const PVector & sub) {
    return PVector{sub.x * lhs, sub.y * lhs};
};

PVector GravitationalForce{0, 9.8};

struct SandProperties {
    int iSandType = 0;
    bool isHover = false;
    SDL_Color Color{};
};

SDL_Color LastColor{0xFF, 0, 0, 0xFF};

vector<vector<SandProperties>> vSandMap(floor(iWindowWidth / iSandSize), vector<SandProperties>(floor((iWindowHeight - 60) / iSandSize)));

vector<vector<PVector>> vSandForce(floor(iWindowWidth / iSandSize), vector<PVector>(floor((iWindowHeight - 60) / iSandSize), {0, 0}));

/*
Sand Identification
    #id     #Name           #Reason
    0       Empty           Empty box
    1       Sand            is Sand
    2       Sand-Summoner   Following cursor
*/

// Functions Area


// TODO: Implementing gravitation pull with forces & further expansion with outer forces ( wind,... )

static int resizingEventWatcher(void* data, const SDL_Event* event) {
    if (event->type == SDL_WINDOWEVENT &&
        event->window.event == SDL_WINDOWEVENT_RESIZED) {
            if (const SDL_Window* win = SDL_GetWindowFromID(event->window.windowID); win == static_cast<SDL_Window *>(data)) {
                windowSize.w = event->window.data1;
                windowSize.h = event->window.data2;
                const int iNewWidth = floor(windowSize.w / iSandSize);
                const int iNewHeight = floor((windowSize.h - 60) / iSandSize);

                vSandMap.resize(iNewWidth);
                vSandForce.resize(iNewWidth);
                for (int i = 0; i < iNewWidth; i++) {
                    vSandMap[i].resize(iNewHeight, {0, false});
                    vSandForce[i].resize(iNewHeight, {0, 0});
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
            if (vSandMap[iKeyBoxX + i][iKeyBoxY + j].iSandType != 1 && iKeyBoxX + i >= 0 && iKeyBoxX + i < vSandMap.size() && iKeyBoxY + j >= 0 && iKeyBoxY + j < vSandMap.at(iKeyBoxX + i).size()) {
                vSandMap[iKeyBoxX + i][iKeyBoxY + j].iSandType = 1;
                if (LastColor.r == 0xFF) {
                    if (LastColor.g == 0 && LastColor.b > 0) {
                        LastColor.b -= 0x1;
                    } else if (LastColor.g < 0xFF && LastColor.b == 0) {
                        LastColor.g += 0x1;
                    }
                }
                if (LastColor.g == 0xFF) {
                    if (LastColor.r > 0 && LastColor.b == 0) {
                        LastColor.r -= 0x1;
                    } else if (LastColor.r == 0 && LastColor.b < 0xFF) {
                        LastColor.b += 0x1;
                    }
                }
                if (LastColor.b == 0xFF) {
                    if (LastColor.r == 0 && LastColor.g > 0) {
                        LastColor.g -= 0x1;
                    } else if (LastColor.r < 0xFF && LastColor.g == 0) {
                        LastColor.r += 0x1;
                    }
                }

                vSandMap[iKeyBoxX + i][iKeyBoxY + j].Color = LastColor;
                vSandForce[iKeyBoxX + i][iKeyBoxY + j] = GravitationalForce;
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
                    SDL_SetRenderDrawColor(renderer, vSandMap[i][j].Color.r - 0x10, vSandMap[i][j].Color.g - 0x10, vSandMap[i][j].Color.b - 0x10, 0xFF);
                    SDL_RenderFillRect(renderer, &gridRect);
                    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
                    SDL_RenderDrawRect(renderer, &gridRect);
                } else {
                    SDL_SetRenderDrawColor(renderer, vSandMap[i][j].Color.r, vSandMap[i][j].Color.g, vSandMap[i][j].Color.b, 0xFF);
                    SDL_RenderFillRect(renderer, &gridRect);
                }
            } else {
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

// TODO: Algorithm changes: run from the middle to both sides instead of left to right

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
                            vSandMap[j][i - 1].Color = vSandMap[j][i].Color;
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
                                    vSandMap[j - 1][i - 1].Color = vSandMap[j][i].Color;
                                } else {
                                    vSandMap[j][i].iSandType = 0;
                                    vSandMap[j + 1][i - 1].iSandType = 1;
                                    vSandMap[j + 1][i - 1].Color = vSandMap[j][i].Color;
                                }
                            } else if (isLeftAvailable) {
                                vSandMap[j][i].iSandType = 0;
                                vSandMap[j - 1][i - 1].iSandType = 1;
                                vSandMap[j - 1][i - 1].Color = vSandMap[j][i].Color;
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

    // TODO: Remove this commented section of code when completed
    // const string sGuild{"Guild: Scroll down to decrease sand summoner size, up to increase | Press '+' to increase speed and '-' of sand falling. \nThis Simulator Was Created by HuyN... Appreciate my works pls..."};
    //
    //
    // constexpr SDL_MessageBoxButtonData messageBoxButton{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Understood."};
    //
    // const SDL_MessageBoxData messageBoxData{SDL_MESSAGEBOX_INFORMATION, window, "Guild to use the simulator.", sGuild.c_str(), 1, &messageBoxButton, nullptr};
    //
    // SDL_ShowMessageBox(&messageBoxData, nullptr);


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
    }

    return EXIT_SUCCESS;
}