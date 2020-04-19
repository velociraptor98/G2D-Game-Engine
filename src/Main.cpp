#include <iostream>
#include"./Constants.h"
#include "./Game.h"

int main(int argc, char *argv[]) 
{
    Game* game =new Game();
    game->init(WINDOW_WIDTH,WINDOW_HEIGHT);
    while(game->IsRunning())
    {
        game->ProcessInput();
        game->Update();
        game->Render();
    }
    game->Destroy();
    return 0;
}
