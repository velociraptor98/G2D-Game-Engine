#include "./SceneLoader.h"
#include <fstream>
#include <iostream>
#include <utility>
#include "./TileMap.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/AnimationComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/CameraFollowComponent.h"
#include "../Components/TextLabelComponent.h"
#include "../Components/AudioComponent.h"
#include "../Components/ScriptComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/TeamComponent.h"
#include "../Components/DamageOnContactComponent.h"
#include "../Components/SolidComponent.h"
#include "../Components/CollisionResponseComponent.h"
#include "../Components/StayInBoundsComponent.h"
#include "../Components/LifetimeComponent.h"
#include "../Components/KeyboardControlledComponent.h"
#include "../Components/ProjectileEmitterComponent.h"

namespace
{
    SDL_Keycode KeyFromName(const TableReader &fields, const char *field)
    {
        const std::string name = fields.String(field, "");
        if (name.empty())
            return SDLK_UNKNOWN;
        const SDL_Keycode key = SDL_GetKeyFromName(name.c_str());
        if (key == SDLK_UNKNOWN)
            fields.Warn(std::string(field) + ": unknown key '" + name + "'");
        return key;
    }

    void LoadKeyboardControl(const TableReader &c, Entity entity)
    {
        KeyboardControlledComponent control(c.Float("speed", 0.0f));
        const char *names[] = {"up", "down", "left", "right"};
        c.WithTable("keys", [&](const TableReader &keys) {
            for (int direction = 0; direction < 4; ++direction)
            {
                if (keys.Has(names[direction]))
                {
                    control.keys[direction].clear();
                    for (const auto &name : keys.StringList(names[direction]))
                    {
                        const SDL_Scancode key = SDL_GetScancodeFromName(name.c_str());
                        if (key == SDL_SCANCODE_UNKNOWN)
                            keys.Warn(std::string(names[direction]) + ": unknown key '" + name + "'");
                        else
                            control.keys[direction].push_back(key);
                    }
                }
            }
        });
        c.WithTable("sprite_rows", [&](const TableReader &rows) {
            for (int direction = 0; direction < 4; ++direction)
                control.spriteRows[direction] = rows.Int(names[direction], KeyboardControlledComponent::NO_SPRITE_ROW);
        });
        entity.AddComponent<KeyboardControlledComponent>(control);
    }

    void LoadProjectileEmitter(const TableReader &c, Entity entity)
    {
        ProjectileTemplate shot;
        c.WithTable("projectile", [&](const TableReader &p) {
            shot.assetId = p.String("texture_id", "");
            shot.width = p.Int("width", shot.width);
            shot.height = p.Int("height", shot.height);
            shot.zIndex = p.Int("z_index", shot.zIndex);
            shot.damage = p.Int("damage", shot.damage);
            shot.lifetimeMs = p.Int("lifetime", shot.lifetimeMs);
        });
        const std::string aim = c.String("aim", "fixed");
        if (aim != "fixed" && aim != "facing")
            c.Warn("aim should be \"fixed\" or \"facing\", not '" + aim + "'");
        entity.AddComponent<ProjectileEmitterComponent>(c.Vec2("projectile_velocity", glm::vec2(0.0f, 0.0f)),
                                                        c.Int("repeat_frequency", 0), KeyFromName(c, "trigger_key"),
                                                        aim == "facing", shot);
    }

    void LoadCollisionResponse(const TableReader &c, Entity entity)
    {
        const std::string name = c.String("on_solid", "none");
        SolidResponse response = SolidResponse::None;
        if (name == "bounce")
            response = SolidResponse::Bounce;
        else if (name == "block")
            response = SolidResponse::Block;
        else if (name == "destroy")
            response = SolidResponse::Destroy;
        else if (name != "none")
            c.Warn("on_solid should be none, bounce, block or destroy, not '" + name + "'");
        entity.AddComponent<CollisionResponseComponent>(response, c.Bool("flip_sprite_on_bounce", false));
    }
}

SceneLoader::SceneLoader() : resolvePath([](const std::string &path) { return path; })
{
    RegisterComponent("transform", [](const TableReader &c, Entity e) {
        e.AddComponent<TransformComponent>(c.Vec2("position", glm::vec2(0.0f, 0.0f)),
                                           c.Vec2("scale", glm::vec2(1.0f, 1.0f)), c.Number("rotation", 0.0));
    });
    RegisterComponent("rigidbody", [](const TableReader &c, Entity e) {
        e.AddComponent<RigidBodyComponent>(c.Vec2("velocity", glm::vec2(0.0f, 0.0f)));
    });
    RegisterComponent("sprite", [](const TableReader &c, Entity e) {
        auto &sprite = e.AddComponent<SpriteComponent>(c.String("texture_id", ""), c.Int("width", 0),
                                                       c.Int("height", 0), c.Int("z_index", 0), c.Bool("fixed", false),
                                                       c.Int("src_rect_x", 0), c.Int("src_rect_y", 0));
        const std::string flip = c.String("flip", "none");
        if (flip == "horizontal")
            sprite.flip = SDL_FLIP_HORIZONTAL;
        else if (flip == "vertical")
            sprite.flip = SDL_FLIP_VERTICAL;
        else if (flip != "none")
            c.Warn("flip should be none, horizontal or vertical, not '" + flip + "'");
    });
    RegisterComponent("animation", [](const TableReader &c, Entity e) {
        e.AddComponent<AnimationComponent>(c.Int("num_frames", 1), c.Int("speed_rate", 1), c.Bool("loop", true));
    });
    RegisterComponent("boxcollider", [](const TableReader &c, Entity e) {
        e.AddComponent<BoxColliderComponent>(c.Int("width", 0), c.Int("height", 0),
                                             c.Vec2("offset", glm::vec2(0.0f, 0.0f)));
    });
    RegisterComponent("camera_follow", [](const TableReader &, Entity e) { e.AddComponent<CameraFollowComponent>(); });
    RegisterComponent("text_label", [](const TableReader &c, Entity e) {
        e.AddComponent<TextLabelComponent>(c.Vec2("position", glm::vec2(0.0f, 0.0f)), c.String("text", ""),
                                           c.String("font_id", ""), c.Color("color", SDL_Color{255, 255, 255, 255}),
                                           c.Bool("fixed", true), c.Bool("centred", false));
    });
    RegisterComponent("audio", [](const TableReader &c, Entity e) {
        e.AddComponent<AudioComponent>(c.String("sound_id", ""), c.Bool("loop", false),
                                       c.Int("volume", MIX_MAX_VOLUME));
    });
    RegisterComponent("health", [](const TableReader &c, Entity e) {
        const int maxHealth = c.Int("max_health", 100);
        e.AddComponent<HealthComponent>(c.Int("health", maxHealth), maxHealth, c.Bool("show_bar", true));
    });
    RegisterComponent("team", [](const TableReader &c, Entity e) {
        e.AddComponent<TeamComponent>(c.String("name", ""));
    });
    RegisterComponent("damage_on_contact", [](const TableReader &c, Entity e) {
        e.AddComponent<DamageOnContactComponent>(c.Int("damage", 0), c.Bool("destroy_on_contact", false));
    });
    RegisterComponent("solid", [](const TableReader &, Entity e) { e.AddComponent<SolidComponent>(); });
    RegisterComponent("collision_response", LoadCollisionResponse);
    RegisterComponent("stay_in_bounds", [](const TableReader &, Entity e) { e.AddComponent<StayInBoundsComponent>(); });
    RegisterComponent("lifetime", [](const TableReader &c, Entity e) {
        e.AddComponent<LifetimeComponent>(c.Int("duration", 0));
    });
    RegisterComponent("keyboard_controller", LoadKeyboardControl);
    RegisterComponent("projectile_emitter", LoadProjectileEmitter);
}

void SceneLoader::RegisterComponent(const std::string &name, ComponentLoader loader)
{
    loaders[name] = std::move(loader);
}

void SceneLoader::SetPathResolver(std::function<std::string(const std::string &)> resolver)
{
    resolvePath = std::move(resolver);
}

void SceneLoader::AddComponent(lua_State *lua, const std::string &name, const std::string &context,
                               Entity entity) const
{
    const std::string componentContext = context + "." + name;
    if (name == "script")
    {
        if (lua_isfunction(lua, -1))
        {
            lua_pushvalue(lua, -1);
            entity.AddComponent<ScriptComponent>(luaL_ref(lua, LUA_REGISTRYINDEX));
        }
        else
        {
            std::cerr << "Scene warning: " << componentContext << " should be a function" << std::endl;
        }
        return;
    }
    auto loader = loaders.find(name);
    if (loader == loaders.end())
    {
        std::cerr << "Scene warning: " << context << ": unknown component '" << name << "'" << std::endl;
        return;
    }
    if (!lua_istable(lua, -1))
    {
        std::cerr << "Scene warning: " << componentContext << " should be a table" << std::endl;
        return;
    }
    const TableReader fields(lua, -1, componentContext);
    loader->second(fields, entity);
    fields.ReportUnknownFields();
}

Entity SceneLoader::CreateEntity(lua_State *lua, int index, const std::string &context, Registry &registry) const
{
    const TableReader definition(lua, index, context);
    if (lua_rawlen(lua, definition.Index()) > 0)
    {
        definition.Warn("has list items; an entity is a table with tag, group and components "
                        "(was a list of entities nested inside the entities list?)");
    }
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
    definition.ReportUnknownFields();
    return entity;
}

void SceneLoader::LoadAssets(const TableReader &scene, AssetManager &assetManager, SDL_Renderer *renderer) const
{
    lua_State *lua = scene.Lua();
    scene.WithTable("assets", [&](const TableReader &assets) {
        for (lua_Integer i = 1; i <= static_cast<lua_Integer>(lua_rawlen(lua, assets.Index())); ++i)
        {
            lua_rawgeti(lua, assets.Index(), i);
            const TableReader asset(lua, -1, assets.Context() + "[" + std::to_string(i) + "]");
            const std::string type = asset.String("type", "");
            const std::string id = asset.String("id", "");
            const std::string file = Resolve(asset.String("file", ""));
            if (type == "texture")
                assetManager.AddTexture(renderer, id, file);
            else if (type == "font")
                assetManager.AddFont(id, file, asset.Int("font_size", 16));
            else if (type == "sound")
                assetManager.AddSound(id, file);
            else
                asset.Warn("unknown asset type '" + type + "'");
            asset.ReportUnknownFields();
            lua_pop(lua, 1);
        }
    });
}

bool SceneLoader::LoadTileMap(const TableReader &scene, Registry &registry, SceneInfo &info) const
{
    bool loaded = true;
    scene.WithTable("tilemap", [&](const TableReader &tilemap) {
        const std::string mapPath = Resolve(tilemap.String("map_file", ""));
        const std::string textureId = tilemap.String("texture_id", "");
        const int tileSize = tilemap.Int("tile_size", 32);
        const float scale = tilemap.Float("scale", 1.0f);
        std::ifstream mapFile(mapPath);
        TileMap map;
        std::string error;
        if (!mapFile || !ParseTileMap(mapFile, map, error))
        {
            std::cerr << "Failed to load map " << mapPath << ": " << (mapFile ? error : "cannot open file")
                      << std::endl;
            loaded = false;
            return;
        }
        CreateTileEntities(map, textureId, tileSize, scale, registry);
        info.worldWidth = static_cast<int>(map.columns * tileSize * scale);
        info.worldHeight = static_cast<int>(map.rows * tileSize * scale);
    });
    return loaded;
}

bool SceneLoader::LoadFile(const std::string &path, lua_State *lua, Registry &registry, AssetManager &assetManager,
                           SDL_Renderer *renderer, SceneInfo &info) const
{
    const std::string resolved = Resolve(path);
    if (luaL_dofile(lua, resolved.c_str()) != LUA_OK)
    {
        std::cerr << "Failed to run " << resolved << ": " << lua_tostring(lua, -1) << std::endl;
        lua_pop(lua, 1);
        return false;
    }
    return LoadFromGlobal(lua, registry, assetManager, renderer, info);
}

bool SceneLoader::LoadFromGlobal(lua_State *lua, Registry &registry, AssetManager &assetManager,
                                 SDL_Renderer *renderer, SceneInfo &info) const
{
    lua_getglobal(lua, "Scene");
    if (!lua_istable(lua, -1))
    {
        std::cerr << "Scene file must define a global table named Scene" << std::endl;
        lua_pop(lua, 1);
        return false;
    }
    const TableReader scene(lua, -1, "Scene");
    scene.Has("on_start");
    scene.Has("on_update");
    scene.Has("on_key");
    info.backgroundColor = scene.Color("background_color", info.backgroundColor);
    scene.WithTable("world", [&](const TableReader &world) {
        info.worldWidth = world.Int("width", 0);
        info.worldHeight = world.Int("height", 0);
    });
    LoadAssets(scene, assetManager, renderer);
    const bool loaded = LoadTileMap(scene, registry, info);
    if (loaded)
    {
        scene.WithTable("entities", [&](const TableReader &entities) {
            for (lua_Integer i = 1; i <= static_cast<lua_Integer>(lua_rawlen(lua, entities.Index())); ++i)
            {
                lua_rawgeti(lua, entities.Index(), i);
                const std::string context = "entities[" + std::to_string(i) + "]";
                if (lua_istable(lua, -1))
                    CreateEntity(lua, -1, context, registry);
                else
                    std::cerr << "Scene warning: " << context << " should be a table" << std::endl;
                lua_pop(lua, 1);
            }
        });
    }
    scene.ReportUnknownFields();
    lua_pop(lua, 1);
    return loaded;
}
