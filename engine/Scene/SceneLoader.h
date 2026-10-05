#ifndef SCENELOADER_H
#define SCENELOADER_H
#include <SDL2/SDL.h>
#include <functional>
#include <lua.hpp>
#include <string>
#include <unordered_map>
#include "../ECS/ECS.h"
#include "../Assets/AssetManager.h"
#include "./TableReader.h"

// World size comes from the tilemap, or `world = { width, height }`; 0 means
// the scene set neither.
struct SceneInfo
{
    int worldWidth = 0;
    int worldHeight = 0;
    SDL_Color backgroundColor = {21, 21, 21, 255};
};

// Builds scenes from Lua. A scene file defines a global table `Scene` with
// optional `assets`, `tilemap`, `entities`, `world`, `background_color`, and
// the callbacks `on_start`, `on_update` and `on_key` (run by the Engine). Each entity has an
// optional `tag`, `group` and a `components` table keyed by component name.
// Every component, built-in or not, is created by a loader registered by name.
class SceneLoader
{
public:
    using ComponentLoader = std::function<void(const TableReader &fields, Entity entity)>;

    SceneLoader();
    // Adds (or replaces) the loader for `name`; games use this to make their
    // own components available to scene files.
    void RegisterComponent(const std::string &name, ComponentLoader loader);
    // Relative paths in scenes are resolved through this (identity by default).
    void SetPathResolver(std::function<std::string(const std::string &)> resolver);

    // Runs the file, then builds the scene from the `Scene` table it defined.
    bool LoadFile(const std::string &path, lua_State *lua, Registry &registry, AssetManager &assetManager,
                  SDL_Renderer *renderer, SceneInfo &info) const;
    bool LoadFromGlobal(lua_State *lua, Registry &registry, AssetManager &assetManager, SDL_Renderer *renderer,
                        SceneInfo &info) const;
    // Creates one entity from the definition table at `index`.
    Entity CreateEntity(lua_State *lua, int index, const std::string &context, Registry &registry) const;

private:
    void LoadAssets(const TableReader &scene, AssetManager &assetManager, SDL_Renderer *renderer) const;
    bool LoadTileMap(const TableReader &scene, Registry &registry, SceneInfo &info) const;
    void AddComponent(lua_State *lua, const std::string &name, const std::string &context, Entity entity) const;
    std::string Resolve(const std::string &path) const { return resolvePath(path); }

    std::unordered_map<std::string, ComponentLoader> loaders;
    std::function<std::string(const std::string &)> resolvePath;
};
#endif
