#ifndef GAME_H
#define GAME_H
#include <SDL2/SDL.h>
#include <memory>
#include <lua.hpp>
#include "./ECS/ECS.h"
#include "./AssetManager.h"
#include "./LevelLoader.h"
#include "./EventBus/EventBus.h"
class Game
{
    private:
    bool isRunning;
    SDL_Window* window;
    SDL_Renderer* renderer;
    Uint32 ticksLastFrame;
    std::unique_ptr<Registry> registry;
    std::unique_ptr<AssetManager> assetManager;
    std::unique_ptr<EventBus> eventBus;
    std::unique_ptr<lua_State, decltype(&lua_close)> lua;
    SDL_Rect camera;
    LevelInfo level;
    bool isDebug;

    public:
    Game();
    ~Game();
    bool IsRunning() const;
    void init(int width,int height);
    bool LoadLevel(int levelNumber);
    void ProcessInput();
    void Update();
    void Render();
    void Destroy();
};
#endif
