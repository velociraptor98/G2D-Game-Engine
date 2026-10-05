#include "./AssetManager.h"
#include <iostream>
AssetManager::AssetManager(EntityManager *manager) : manager(manager)
{
    ;
}
AssetManager::~AssetManager()
{
    clearData();
}
void AssetManager::clearData()
{
    for(auto& texture : textures)
    {
        SDL_DestroyTexture(texture.second);
    }
    textures.clear();
}
void AssetManager::addTexture(std::string textureId, const char *filePath)
{
    SDL_Texture *texture = TextureManager::LoadTexture(filePath);
    if(!texture)
    {
        return;
    }
    auto existing = textures.find(textureId);
    if(existing != textures.end())
    {
        SDL_DestroyTexture(existing->second);
        existing->second = texture;
        return;
    }
    textures.emplace(textureId, texture);
}
SDL_Texture *AssetManager::getTexture(std::string textureId)
{
    auto it = textures.find(textureId);
    if(it == textures.end())
    {
        std::cerr<<"Texture not found: "<<textureId<<std::endl;
        return nullptr;
    }
    return it->second;
}
