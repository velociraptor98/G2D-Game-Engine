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
glm::vec2 projectilePos = glm::vec2(0.0f,0.0f);
glm::vec2 projectileVel = glm::vec2(20.0f,20.0f);

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
    //quick way to make sure update doesn't exceed the target.
    int waitTime = FRAME_TARGET - (SDL_GetTicks()-ticksLastFrame);
    if(waitTime > 0 && waitTime <= waitTime)
    {
        SDL_Delay(waitTime);
    }
    float deltaTime = (SDL_GetTicks()-ticksLastFrame)/1000.0f;
    //Clamping the delta time 
    deltaTime = (deltaTime>0.05f)?0.05f : deltaTime;
    ticksLastFrame = SDL_GetTicks();
    projectilePos =glm::vec2(projectilePos.x+(projectileVel.x * deltaTime),(projectileVel.y*deltaTime)+projectilePos.y);
}
void Game::Render()
{
    SDL_SetRenderDrawColor(renderer,21,21,21,255);
    SDL_RenderClear(renderer);
    SDL_Rect projectile {(int)projectilePos.x,(int)projectilePos.y,10,10};
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

