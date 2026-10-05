#ifndef LEVELLOADER_H
#define LEVELLOADER_H
#include <SDL2/SDL.h>
#include <istream>
#include <lua.hpp>
#include <string>
#include <vector>
#include "./ECS/ECS.h"
#include "./AssetManager.h"

// A .map file holds two comma-separated grids of equal size, separated by a
// blank line. In the first, each two-digit code "RC" picks the tile at row R,
// column C of the tileset. In the second, 1 marks a solid (obstacle) tile.
struct TileMap
{
    int columns = 0;
    int rows = 0;
    std::vector<SDL_Point> tiles;
    std::vector<bool> obstacles;
    const SDL_Point &TileAt(int column, int row) const { return tiles[row * columns + column]; }
    bool IsObstacle(int column, int row) const { return obstacles[row * columns + column]; }
};

bool ParseTileMap(std::istream &input, TileMap &map, std::string &error);

struct LevelInfo
{
    int mapWidth = 0;
    int mapHeight = 0;
};

// Levels are Lua files (assets/scripts/Level<N>.lua) that define a global
// `Level` table with `assets`, `tilemap` and `entities`; see Level1.lua.
class LevelLoader
{
public:
    static bool LoadLevel(int levelNumber, lua_State *lua, Registry &registry, AssetManager &assetManager,
                          SDL_Renderer *renderer, LevelInfo &level);
    // Builds the level from the `Level` table already defined in `lua`.
    static bool LoadLevelFromLua(lua_State *lua, Registry &registry, AssetManager &assetManager,
                                 SDL_Renderer *renderer, LevelInfo &level);
    static void CreateTileEntities(const TileMap &map, const std::string &tilesetId, int tileSize, float tileScale,
                                   Registry &registry);
};
#endif
