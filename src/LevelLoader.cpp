#include "./LevelLoader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include "./Components/TransformComponent.h"
#include "./Components/RigidBodyComponent.h"
#include "./Components/SpriteComponent.h"
#include "./Components/AnimationComponent.h"
#include "./Components/CameraFollowComponent.h"
#include "./Components/KeyboardControlledComponent.h"
#include "./Components/BoxColliderComponent.h"
#include "./Components/HealthComponent.h"
#include "./Components/ProjectileEmitterComponent.h"
#include "./Components/TextLabelComponent.h"
#include "./Components/AudioComponent.h"
#include "./Components/ScriptComponent.h"

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

void LevelLoader::CreateTileEntities(const TileMap &map, const std::string &tilesetId, int tileSize, float tileScale,
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
                tile.Group("obstacles");
            }
            else
            {
                tile.Group("tiles");
            }
        }
    }
}

namespace
{
    // Reads fields from the Lua table at absolute stack index `table`. A missing
    // field gives the fallback; a field of the wrong type also warns, naming
    // `context` so level authors can find the typo.
    class TableReader
    {
    public:
        TableReader(lua_State *lua, int table, std::string context)
            : lua(lua), table(lua_absindex(lua, table)), context(std::move(context))
        {
        }

        double Number(const char *key, double fallback) const
        {
            lua_getfield(lua, table, key);
            double value = fallback;
            if (lua_isnumber(lua, -1))
            {
                value = lua_tonumber(lua, -1);
            }
            else if (!lua_isnil(lua, -1))
            {
                WrongType(key, "a number");
            }
            lua_pop(lua, 1);
            return value;
        }
        int Int(const char *key, int fallback) const { return static_cast<int>(Number(key, fallback)); }
        float Float(const char *key, float fallback) const { return static_cast<float>(Number(key, fallback)); }

        bool Bool(const char *key, bool fallback) const
        {
            lua_getfield(lua, table, key);
            bool value = fallback;
            if (lua_isboolean(lua, -1))
            {
                value = lua_toboolean(lua, -1);
            }
            else if (!lua_isnil(lua, -1))
            {
                WrongType(key, "a boolean");
            }
            lua_pop(lua, 1);
            return value;
        }

        std::string String(const char *key, const std::string &fallback) const
        {
            lua_getfield(lua, table, key);
            std::string value = fallback;
            if (lua_type(lua, -1) == LUA_TSTRING)
            {
                value = lua_tostring(lua, -1);
            }
            else if (!lua_isnil(lua, -1))
            {
                WrongType(key, "a string");
            }
            lua_pop(lua, 1);
            return value;
        }

        glm::vec2 Vec2(const char *key, glm::vec2 fallback) const
        {
            glm::vec2 value = fallback;
            if (PushTable(key))
            {
                const TableReader inner(lua, -1, context + "." + key);
                value = glm::vec2(inner.Float("x", fallback.x), inner.Float("y", fallback.y));
                lua_pop(lua, 1);
            }
            return value;
        }

        SDL_Color Color(const char *key, SDL_Color fallback) const
        {
            SDL_Color value = fallback;
            if (PushTable(key))
            {
                const TableReader inner(lua, -1, context + "." + key);
                value = SDL_Color{static_cast<Uint8>(inner.Int("r", fallback.r)),
                                  static_cast<Uint8>(inner.Int("g", fallback.g)),
                                  static_cast<Uint8>(inner.Int("b", fallback.b)),
                                  static_cast<Uint8>(inner.Int("a", fallback.a))};
                lua_pop(lua, 1);
            }
            return value;
        }

        // Pushes the field and returns true if it is a table; pushes nothing otherwise.
        bool PushTable(const char *key) const
        {
            lua_getfield(lua, table, key);
            if (lua_istable(lua, -1))
            {
                return true;
            }
            if (!lua_isnil(lua, -1))
            {
                WrongType(key, "a table");
            }
            lua_pop(lua, 1);
            return false;
        }

    private:
        void WrongType(const char *key, const char *expected) const
        {
            std::cerr << "Level warning: " << context << "." << key << " should be " << expected << std::endl;
        }

        lua_State *lua;
        int table;
        std::string context;
    };

    void LoadAssets(lua_State *lua, const TableReader &level, AssetManager &assetManager, SDL_Renderer *renderer)
    {
        if (!level.PushTable("assets"))
        {
            return;
        }
        const int assets = lua_gettop(lua);
        for (lua_Integer i = 1; i <= static_cast<lua_Integer>(lua_rawlen(lua, assets)); ++i)
        {
            lua_rawgeti(lua, assets, i);
            const TableReader asset(lua, -1, "assets[" + std::to_string(i) + "]");
            const std::string type = asset.String("type", "");
            const std::string id = asset.String("id", "");
            const std::string file = asset.String("file", "");
            if (type == "texture")
                assetManager.AddTexture(renderer, id, file);
            else if (type == "font")
                assetManager.AddFont(id, file, asset.Int("font_size", 16));
            else if (type == "sound")
                assetManager.AddSound(id, file);
            else
                std::cerr << "Level warning: assets[" << i << "] has unknown type '" << type << "'" << std::endl;
            lua_pop(lua, 1);
        }
        lua_pop(lua, 1);
    }

    bool LoadTileMap(lua_State *lua, const TableReader &level, Registry &registry, LevelInfo &info)
    {
        if (!level.PushTable("tilemap"))
        {
            return true;
        }
        const TableReader tilemap(lua, -1, "tilemap");
        const std::string mapPath = tilemap.String("map_file", "");
        const std::string textureId = tilemap.String("texture_id", "");
        const int tileSize = tilemap.Int("tile_size", 32);
        const float scale = tilemap.Float("scale", 1.0f);
        lua_pop(lua, 1);

        std::ifstream mapFile(mapPath);
        TileMap map;
        std::string error;
        if (!mapFile || !ParseTileMap(mapFile, map, error))
        {
            std::cerr << "Failed to load map " << mapPath << ": " << (mapFile ? error : "cannot open file")
                      << std::endl;
            return false;
        }
        LevelLoader::CreateTileEntities(map, textureId, tileSize, scale, registry);
        info.mapWidth = static_cast<int>(map.columns * tileSize * scale);
        info.mapHeight = static_cast<int>(map.rows * tileSize * scale);
        return true;
    }

    void AddComponent(lua_State *lua, const std::string &name, const std::string &context, Entity entity)
    {
        if (name == "script")
        {
            if (lua_isfunction(lua, -1))
            {
                lua_pushvalue(lua, -1);
                entity.AddComponent<ScriptComponent>(luaL_ref(lua, LUA_REGISTRYINDEX));
            }
            else
            {
                std::cerr << "Level warning: " << context << ".script should be a function" << std::endl;
            }
            return;
        }
        if (!lua_istable(lua, -1))
        {
            std::cerr << "Level warning: " << context << "." << name << " should be a table" << std::endl;
            return;
        }
        const TableReader c(lua, -1, context + "." + name);
        if (name == "transform")
            entity.AddComponent<TransformComponent>(c.Vec2("position", glm::vec2(0.0f, 0.0f)),
                                                    c.Vec2("scale", glm::vec2(1.0f, 1.0f)),
                                                    c.Number("rotation", 0.0));
        else if (name == "rigidbody")
            entity.AddComponent<RigidBodyComponent>(c.Vec2("velocity", glm::vec2(0.0f, 0.0f)));
        else if (name == "sprite")
            entity.AddComponent<SpriteComponent>(c.String("texture_id", ""), c.Int("width", 0), c.Int("height", 0),
                                                 c.Int("z_index", 0), c.Bool("fixed", false),
                                                 c.Int("src_rect_x", 0), c.Int("src_rect_y", 0));
        else if (name == "animation")
            entity.AddComponent<AnimationComponent>(c.Int("num_frames", 1), c.Int("speed_rate", 1),
                                                    c.Bool("loop", true));
        else if (name == "boxcollider")
            entity.AddComponent<BoxColliderComponent>(c.Int("width", 0), c.Int("height", 0),
                                                      c.Vec2("offset", glm::vec2(0.0f, 0.0f)));
        else if (name == "health")
            entity.AddComponent<HealthComponent>(c.Int("health_percentage", 100));
        else if (name == "projectile_emitter")
            entity.AddComponent<ProjectileEmitterComponent>(
                c.Vec2("projectile_velocity", glm::vec2(0.0f, 0.0f)), c.Int("repeat_frequency", 0),
                c.Int("projectile_duration", 10000), c.Int("hit_percentage_damage", 10), c.Bool("friendly", false),
                c.String("texture_id", ""));
        else if (name == "keyboard_controller")
            entity.AddComponent<KeyboardControlledComponent>(c.Float("speed", 0.0f));
        else if (name == "camera_follow")
            entity.AddComponent<CameraFollowComponent>();
        else if (name == "text_label")
            entity.AddComponent<TextLabelComponent>(c.Vec2("position", glm::vec2(0.0f, 0.0f)),
                                                    c.String("text", ""), c.String("font_id", ""),
                                                    c.Color("color", SDL_Color{255, 255, 255, 255}),
                                                    c.Bool("fixed", true), c.Bool("centred", false));
        else if (name == "audio")
            entity.AddComponent<AudioComponent>(c.String("sound_id", ""), c.Bool("loop", false),
                                                c.Int("volume", MIX_MAX_VOLUME));
        else
            std::cerr << "Level warning: " << context << " has unknown component '" << name << "'" << std::endl;
    }

    void LoadEntities(lua_State *lua, const TableReader &level, Registry &registry)
    {
        if (!level.PushTable("entities"))
        {
            return;
        }
        const int entities = lua_gettop(lua);
        for (lua_Integer i = 1; i <= static_cast<lua_Integer>(lua_rawlen(lua, entities)); ++i)
        {
            lua_rawgeti(lua, entities, i);
            const std::string context = "entities[" + std::to_string(i) + "]";
            if (!lua_istable(lua, -1))
            {
                std::cerr << "Level warning: " << context << " should be a table" << std::endl;
                lua_pop(lua, 1);
                continue;
            }
            const TableReader definition(lua, -1, context);
            Entity entity = registry.CreateEntity();
            const std::string tag = definition.String("tag", "");
            const std::string group = definition.String("group", "");
            if (!tag.empty())
                entity.Tag(tag);
            if (!group.empty())
                entity.Group(group);
            if (definition.PushTable("components"))
            {
                const int components = lua_gettop(lua);
                lua_pushnil(lua);
                while (lua_next(lua, components) != 0)
                {
                    if (lua_type(lua, -2) == LUA_TSTRING)
                    {
                        AddComponent(lua, lua_tostring(lua, -2), context, entity);
                    }
                    lua_pop(lua, 1);
                }
                lua_pop(lua, 1);
            }
            lua_pop(lua, 1);
        }
        lua_pop(lua, 1);
    }
}

bool LevelLoader::LoadLevel(int levelNumber, lua_State *lua, Registry &registry, AssetManager &assetManager,
                            SDL_Renderer *renderer, LevelInfo &level)
{
    const std::string path = "./assets/scripts/Level" + std::to_string(levelNumber) + ".lua";
    if (luaL_dofile(lua, path.c_str()) != LUA_OK)
    {
        std::cerr << "Failed to run " << path << ": " << lua_tostring(lua, -1) << std::endl;
        lua_pop(lua, 1);
        return false;
    }
    return LoadLevelFromLua(lua, registry, assetManager, renderer, level);
}

bool LevelLoader::LoadLevelFromLua(lua_State *lua, Registry &registry, AssetManager &assetManager,
                                   SDL_Renderer *renderer, LevelInfo &level)
{
    lua_getglobal(lua, "Level");
    if (!lua_istable(lua, -1))
    {
        std::cerr << "Level script must define a global table named Level" << std::endl;
        lua_pop(lua, 1);
        return false;
    }
    const TableReader levelTable(lua, -1, "Level");
    LoadAssets(lua, levelTable, assetManager, renderer);
    const bool mapLoaded = LoadTileMap(lua, levelTable, registry, level);
    if (mapLoaded)
    {
        LoadEntities(lua, levelTable, registry);
    }
    lua_pop(lua, 1);
    return mapLoaded;
}
