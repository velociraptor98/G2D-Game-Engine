#include "./TileMap.h"
#include <cctype>
#include <sstream>
#include "../Components/TransformComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/SolidComponent.h"

namespace
{
    std::vector<std::string> SplitCells(const std::string &line)
    {
        std::vector<std::string> cells;
        std::stringstream stream(line);
        std::string cell;
        while (std::getline(stream, cell, ','))
        {
            const auto first = cell.find_first_not_of(" \t\r");
            if (first != std::string::npos)
            {
                cells.push_back(cell.substr(first, cell.find_last_not_of(" \t\r") - first + 1));
            }
        }
        return cells;
    }

    bool IsBlank(const std::string &line)
    {
        return line.find_first_not_of(" \t\r") == std::string::npos;
    }
}

bool ParseTileMap(std::istream &input, TileMap &map, std::string &error)
{
    map = TileMap();
    std::string line;
    int lineNumber = 0;
    int section = 0;
    int obstacleRows = 0;
    while (std::getline(input, line))
    {
        ++lineNumber;
        if (IsBlank(line))
        {
            if (map.rows > 0)
            {
                section = 1;
            }
            continue;
        }
        const auto cells = SplitCells(line);
        if (map.columns == 0)
        {
            map.columns = static_cast<int>(cells.size());
        }
        if (static_cast<int>(cells.size()) != map.columns)
        {
            error = "line " + std::to_string(lineNumber) + ": expected " + std::to_string(map.columns) +
                    " cells, found " + std::to_string(cells.size());
            return false;
        }
        for (const auto &cell : cells)
        {
            if (section == 0)
            {
                if (cell.size() != 2 || !isdigit(cell[0]) || !isdigit(cell[1]))
                {
                    error = "line " + std::to_string(lineNumber) + ": bad tile code '" + cell + "'";
                    return false;
                }
                map.tiles.push_back(SDL_Point{cell[1] - '0', cell[0] - '0'});
            }
            else
            {
                if (cell != "0" && cell != "1")
                {
                    error = "line " + std::to_string(lineNumber) + ": bad obstacle flag '" + cell + "'";
                    return false;
                }
                map.obstacles.push_back(cell == "1");
            }
        }
        if (section == 0)
        {
            ++map.rows;
        }
        else
        {
            ++obstacleRows;
        }
    }
    if (map.rows == 0)
    {
        error = "map has no tiles";
        return false;
    }
    if (obstacleRows == 0)
    {
        map.obstacles.assign(map.tiles.size(), false);
    }
    else if (obstacleRows != map.rows)
    {
        error = "obstacle grid has " + std::to_string(obstacleRows) + " rows, tile grid has " +
                std::to_string(map.rows);
        return false;
    }
    return true;
}

void CreateTileEntities(const TileMap &map, const std::string &tilesetId, int tileSize, float tileScale,
                        Registry &registry)
{
    for (int row = 0; row < map.rows; ++row)
    {
        for (int column = 0; column < map.columns; ++column)
        {
            const SDL_Point &source = map.TileAt(column, row);
            Entity tile = registry.CreateEntity();
            tile.AddComponent<TransformComponent>(
                glm::vec2(column * tileSize * tileScale, row * tileSize * tileScale), glm::vec2(tileScale, tileScale));
            tile.AddComponent<SpriteComponent>(tilesetId, tileSize, tileSize, 0, false, source.x * tileSize,
                                               source.y * tileSize);
            if (map.IsObstacle(column, row))
            {
                tile.AddComponent<BoxColliderComponent>(tileSize, tileSize);
                tile.AddComponent<SolidComponent>();
            }
        }
    }
}
