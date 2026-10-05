#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H
#include <map>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
class AssetManager
{
public:
    AssetManager() = default;
    AssetManager(const AssetManager &) = delete;
    AssetManager &operator=(const AssetManager &) = delete;
    ~AssetManager();
    void ClearAssets();
    void AddTexture(SDL_Renderer *renderer, const std::string &textureId, const std::string &filePath);
    SDL_Texture *GetTexture(const std::string &textureId) const;
    // Requires TTF_Init. A font file loaded at two sizes needs two ids.
    void AddFont(const std::string &fontId, const std::string &filePath, int pointSize);
    TTF_Font *GetFont(const std::string &fontId) const;
    // Requires Mix_OpenAudio.
    void AddSound(const std::string &soundId, const std::string &filePath);
    Mix_Chunk *GetSound(const std::string &soundId) const;

private:
    std::map<std::string, SDL_Texture *> textures;
    std::map<std::string, TTF_Font *> fonts;
    std::map<std::string, Mix_Chunk *> sounds;
};
#endif
