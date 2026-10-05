#include "./TextureManager.h"
#include <iostream>
SDL_Texture* TextureManager::LoadTexture(const char* fileName)
{
    SDL_Surface* surface = IMG_Load(fileName);
    if(!surface)
    {
        std::cerr<<"Failed to load image "<<fileName<<": "<<IMG_GetError()<<std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(Game::renderer,surface);
    SDL_FreeSurface(surface);
    if(!texture)
    {
        std::cerr<<"Failed to create texture from "<<fileName<<": "<<SDL_GetError()<<std::endl;
    }
    return texture;
}
void TextureManager::Draw(SDL_Texture* texture,SDL_Rect sourceRectangle,SDL_Rect destinationRectangle,SDL_RendererFlip flip)
{
    SDL_RenderCopyEx(Game::renderer,texture,&sourceRectangle,&destinationRectangle,0.0,nullptr,flip);
}
