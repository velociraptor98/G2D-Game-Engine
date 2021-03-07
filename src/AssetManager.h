#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H
#include <map>
#include <string>
#include "./TextureManager.h"
#include "./EntityManager.h"
class AssetManager
{
public:
    AssetManager(EntityManager *manager);
    ~AssetManager();
    void clearData();
    void addTexture(std::string textureId, const char *filePath);
    SDL_Texture *getTexture(std::string textureId);

private:
    EntityManager *manager;
    std::map<std::string, SDL_Texture *> textures;
};
#endif