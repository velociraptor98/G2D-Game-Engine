#include "./AssetManager.h"
#include <SDL2/SDL_image.h>
#include <iostream>
AssetManager::~AssetManager()
{
    ClearAssets();
}
void AssetManager::ClearAssets()
{
    for (auto &texture : textures)
    {
        SDL_DestroyTexture(texture.second);
    }
    textures.clear();
    for (auto &font : fonts)
    {
        TTF_CloseFont(font.second);
    }
    fonts.clear();
    if (!sounds.empty())
    {
        // A chunk must not be freed while a channel is still playing it.
        Mix_HaltChannel(-1);
    }
    for (auto &sound : sounds)
    {
        Mix_FreeChunk(sound.second);
    }
    sounds.clear();
}
void AssetManager::AddTexture(SDL_Renderer *renderer, const std::string &textureId, const std::string &filePath)
{
    SDL_Surface *surface = IMG_Load(filePath.c_str());
    if (!surface)
    {
        std::cerr << "Failed to load image " << filePath << ": " << IMG_GetError() << std::endl;
        return;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture)
    {
        std::cerr << "Failed to create texture from " << filePath << ": " << SDL_GetError() << std::endl;
        return;
    }
    auto existing = textures.find(textureId);
    if (existing != textures.end())
    {
        SDL_DestroyTexture(existing->second);
        existing->second = texture;
        return;
    }
    textures.emplace(textureId, texture);
}
SDL_Texture *AssetManager::GetTexture(const std::string &textureId) const
{
    auto it = textures.find(textureId);
    return it == textures.end() ? nullptr : it->second;
}
void AssetManager::AddFont(const std::string &fontId, const std::string &filePath, int pointSize)
{
    TTF_Font *font = TTF_OpenFont(filePath.c_str(), pointSize);
    if (!font)
    {
        std::cerr << "Failed to load font " << filePath << ": " << TTF_GetError() << std::endl;
        return;
    }
    auto existing = fonts.find(fontId);
    if (existing != fonts.end())
    {
        TTF_CloseFont(existing->second);
        existing->second = font;
        return;
    }
    fonts.emplace(fontId, font);
}
TTF_Font *AssetManager::GetFont(const std::string &fontId) const
{
    auto it = fonts.find(fontId);
    return it == fonts.end() ? nullptr : it->second;
}
void AssetManager::AddSound(const std::string &soundId, const std::string &filePath)
{
    Mix_Chunk *sound = Mix_LoadWAV(filePath.c_str());
    if (!sound)
    {
        std::cerr << "Failed to load sound " << filePath << ": " << Mix_GetError() << std::endl;
        return;
    }
    auto existing = sounds.find(soundId);
    if (existing != sounds.end())
    {
        Mix_HaltChannel(-1);
        Mix_FreeChunk(existing->second);
        existing->second = sound;
        return;
    }
    sounds.emplace(soundId, sound);
}
Mix_Chunk *AssetManager::GetSound(const std::string &soundId) const
{
    auto it = sounds.find(soundId);
    return it == sounds.end() ? nullptr : it->second;
}
