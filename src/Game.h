#ifndef GAME_H
#define GAME_H
#include <SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<SDL2/SDL_ttf.h>
#include "../lib/glm/glm.hpp"
#include "./EntityManager.h"

class Game
{
    private:
    bool isRunning;
    SDL_Window* window;
    
    public:
    static SDL_Renderer* renderer;
    int ticksLastFrame;
    Game();
    ~Game();
    bool IsRunning() const;
    void init(int width,int height);
    void LoadLevel(int levelNumber);
    void ProcessInput();
    void Update();
    void Render();
    void Destroy();
};
#endif