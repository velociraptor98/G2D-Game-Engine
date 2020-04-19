#include "./Game.h"
#include "./Constants.h"
#include <iostream>
Game::Game()
{
    (*this).isRunning = false; 
}
Game::~Game()
{

}
bool Game::IsRunning() const
{
    return (*this).isRunning;
}
float projectileX =0.0f;
float projectileY = 0.0f;
float velocityX = 0.005f;
float velocityY = 0.005f;

void Game::init(int width,int height)
{
    if(SDL_Init(SDL_INIT_EVERYTHING)!=0)
    {
        std::cerr << "Error initializing SDL 2"<<std::endl;
        return ;
    }
    window  = SDL_CreateWindow(
        NULL,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_BORDERLESS
    ); 
    if(!window)
    {
        std::cerr<<"error in window creation"<<std::endl;
        return;
    }
    renderer = SDL_CreateRenderer(window,-1,0);
    if(!renderer)
    {
        std::cerr<<"Failed to create renderer"<<std::endl;
    }
    isRunning = true;
    return;
}
void Game::ProcessInput()
{
    SDL_Event event;
    SDL_PollEvent(&event);
    switch(event.type)
    {
        case SDL_QUIT:
        isRunning = false;
        break;
        case SDL_KEYDOWN:
        if(event.key.keysym.sym == SDLK_ESCAPE)
        {
            isRunning =false;
        }
        break;
        default:
        break;
    }
}
void Game::Update()
{
    projectileX = projectileX+velocityX;
    projectileY = projectileY + velocityY;
}
void Game::Render()
{
    SDL_SetRenderDrawColor(renderer,21,21,21,255);
    SDL_RenderClear(renderer);
    SDL_Rect projectile {(int)projectileX,(int)projectileY,10,10};
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderFillRect(renderer,&projectile);
    SDL_RenderPresent(renderer);
}
void Game::Destroy()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

