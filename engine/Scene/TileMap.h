#ifndef TILEMAP_H
#define TILEMAP_H
#include <SDL2/SDL.h>
#include <istream>
#include <string>
#include <vector>
#include "../ECS/ECS.h"

// A .map file holds two comma-separated grids of equal size, separated by a
// blank line. In the first, each two-digit code "RC" picks the tile at row R,
// column C of the tileset. In the second (optional), 1 marks a solid tile.
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

// One entity per tile; solid tiles also get a collider and SolidComponent.
void CreateTileEntities(const TileMap &map, const std::string &tilesetId, int tileSize, float tileScale,
                        Registry &registry);
#endif
