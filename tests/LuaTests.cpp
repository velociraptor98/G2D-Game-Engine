#include "./TestFramework.h"
#include "./SDLTestHelpers.h"
#include "./EngineTestHelpers.h"
#include <iostream>
#include <lua.hpp>
#include <sstream>
#include "ECS/ECS.h"
#include "Assets/AssetManager.h"
#include "Scene/SceneLoader.h"
#include "Scripting/LuaBindings.h"
#include "Components/TransformComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/HealthComponent.h"
#include "Components/TextLabelComponent.h"
#include "Components/ScriptComponent.h"
#include "Components/ProjectileEmitterComponent.h"
#include "Components/SolidComponent.h"
#include "Systems/ScriptSystem.h"

namespace
{
    struct LuaWorld
    {
        DummyAudio audio;
        Registry registry;
        AssetManager assets;
        SoftwareRenderTarget target{8, 8};
        lua_State *lua = luaL_newstate();
        ScriptSystem *scripts = nullptr;
        SceneInfo level;
        SceneLoader loader;

        LuaWorld()
        {
            luaL_openlibs(lua);
            RegisterLuaBindings(lua, registry);
            scripts = &registry.AddSystem<ScriptSystem>(lua);
        }
        ~LuaWorld()
        {
            assets.ClearAssets();
            lua_close(lua);
        }
        bool Load(const std::string &source)
        {
            if (luaL_dostring(lua, source.c_str()) != LUA_OK)
            {
                std::cerr << "  lua: " << lua_tostring(lua, -1) << std::endl;
                lua_pop(lua, 1);
                return false;
            }
            const bool loaded = loader.LoadFromGlobal(lua, registry, assets, target.Renderer(), level);
            registry.Update();
            return loaded;
        }
        bool Run(const std::string &source)
        {
            const bool ok = luaL_dostring(lua, source.c_str()) == LUA_OK;
            if (!ok)
            {
                lua_pop(lua, 1);
            }
            return ok;
        }
        lua_Integer GlobalInteger(const char *name)
        {
            lua_getglobal(lua, name);
            const lua_Integer value = lua_tointeger(lua, -1);
            lua_pop(lua, 1);
            return value;
        }
        bool GlobalIsNil(const char *name)
        {
            lua_getglobal(lua, name);
            const bool isNil = lua_isnil(lua, -1);
            lua_pop(lua, 1);
            return isNil;
        }
    };
}

namespace
{
    class SpriteCounter : public System
    {
    public:
        SpriteCounter() { RequireComponent<SpriteComponent>(); }
    };
    class SolidCounter : public System
    {
    public:
        SolidCounter() { RequireComponent<SolidComponent>(); }
    };
}

TEST(LuaLevelCreatesEntitiesWithTheirComponentsTagsAndGroups)
{
    LuaWorld world;
    REQUIRE(world.Load(R"(
        Scene = { entities = {
            { tag = "player", components = {
                transform = { position = { x = 10, y = 20 }, scale = { x = 2, y = 3 }, rotation = 45 },
                rigidbody = { velocity = { x = 5, y = -5 } },
                sprite = { texture_id = "chopper", width = 32, height = 16, z_index = 3, src_rect_y = 16 },
                health = { health = 80, max_health = 120 },
            } },
            { group = "enemies", components = { health = {} } },
            { components = { text_label = { text = "HI", font_id = "f", color = { r = 1, g = 2, b = 3 } } } },
        } }
    )"));
    auto player = world.registry.GetEntityByTag("player");
    REQUIRE(player.has_value());
    const auto &transform = player->GetComponent<TransformComponent>();
    CHECK_EQ(transform.position.x, 10.0f);
    CHECK_EQ(transform.scale.y, 3.0f);
    CHECK_EQ(transform.rotation, 45.0);
    CHECK_EQ(player->GetComponent<RigidBodyComponent>().velocity.y, -5.0f);
    const auto &sprite = player->GetComponent<SpriteComponent>();
    CHECK_EQ(sprite.assetId, std::string("chopper"));
    CHECK_EQ(sprite.height, 16);
    CHECK_EQ(sprite.zIndex, 3);
    CHECK_EQ(sprite.srcRect.y, 16);
    CHECK_EQ(player->GetComponent<HealthComponent>().health, 80);
    CHECK_EQ(player->GetComponent<HealthComponent>().maxHealth, 120);

    auto enemies = world.registry.GetEntitiesByGroup("enemies");
    REQUIRE(enemies.size() == 1u);
    CHECK_EQ(enemies[0].GetComponent<HealthComponent>().health, 100);
}

TEST(LuaSceneWithoutASceneTableFails)
{
    LuaWorld world;
    CapturedErrors errors;
    CHECK(!world.Load("Something = {}"));
    CHECK(errors.str().find("global table named Scene") != std::string::npos);
}

TEST(MissingSceneFileFails)
{
    LuaWorld world;
    CapturedErrors errors;
    CHECK(!world.loader.LoadFile("./nope/missing.lua", world.lua, world.registry, world.assets,
                                 world.target.Renderer(), world.level));
    CHECK(errors.str().find("missing.lua") != std::string::npos);
}

TEST(LuaLevelWarnsAboutTyposAndWrongTypesButKeepsGoing)
{
    LuaWorld world;
    CapturedErrors errors;
    REQUIRE(world.Load(R"(
        Scene = { entities = {
            { tag = "thing", components = {
                transfrom = {},
                health = { max_health = "lots", max_helth = 5 },
                sprite = { width = 8, z_idx = 2 },
            }, compnents = {
            } },
        } }
    )"));
    CHECK(errors.str().find("entities[1]: unknown component 'transfrom'") != std::string::npos);
    CHECK(errors.str().find("entities[1].health: max_health should be a number") != std::string::npos);
    CHECK(errors.str().find("entities[1].health: unknown field 'max_helth'") != std::string::npos);
    CHECK(errors.str().find("entities[1].sprite: unknown field 'z_idx'") != std::string::npos);
    CHECK(errors.str().find("entities[1]: unknown field 'compnents'") != std::string::npos);
    auto thing = world.registry.GetEntityByTag("thing");
    REQUIRE(thing.has_value());
    CHECK_EQ(thing->GetComponent<HealthComponent>().maxHealth, 100);
}

TEST(RegisteringABuiltInNameReplacesItsLoader)
{
    LuaWorld world;
    world.loader.RegisterComponent("health", [](const TableReader &fields, Entity entity) {
        entity.AddComponent<HealthComponent>(fields.Int("hit_points", 1) * 10, 1000);
    });
    REQUIRE(world.Load(R"(Scene = { entities = { { tag = "tough", components = { health = { hit_points = 7 } } } } })"));
    const auto &health = world.registry.GetEntityByTag("tough")->GetComponent<HealthComponent>();
    CHECK_EQ(health.health, 70);
    CHECK_EQ(health.maxHealth, 1000);
}

TEST(ANestedListOfEntitiesIsReported)
{
    LuaWorld world;
    CapturedErrors errors;
    REQUIRE(world.Load(R"(
        Scene = { entities = {
            { components = { transform = {} } },
            { { components = { transform = {} } }, { components = { transform = {} } } },
        } }
    )"));
    CHECK(errors.str().find("entities[2]: has list items") != std::string::npos);
    CHECK(errors.str().find("entities[1]") == std::string::npos);
}

TEST(LuaLevelReadsTheTilemapAndMapSize)
{
    LuaWorld world;
    REQUIRE(world.Load(R"(
        Scene = { tilemap = { map_file = "./assets/tilemaps/jungle.map", texture_id = "tiles", tile_size = 32, scale = 2 } }
    )"));
    CHECK_EQ(world.level.worldWidth, 1600);
    CHECK_EQ(world.level.worldHeight, 1280);
    CHECK_EQ(world.registry.AddSystem<SpriteCounter>().GetEntities().size(), 0u);
    world.registry.Update();
    CHECK_EQ(world.registry.GetSystem<SpriteCounter>().GetEntities().size(), 500u);
    CHECK_EQ(world.registry.AddSystem<SolidCounter>().GetEntities().size(), 0u);
    world.registry.Update();
    CHECK_EQ(world.registry.GetSystem<SolidCounter>().GetEntities().size(), 86u);
}

TEST(LuaLevelWithAMissingMapFileFails)
{
    LuaWorld world;
    CapturedErrors errors;
    CHECK(!world.Load(R"(Scene = { tilemap = { map_file = "./nope.map" } })"));
    CHECK(errors.str().find("nope.map") != std::string::npos);
}

TEST(ScriptsRunEveryFrameWithDeltaTimeAndElapsedTime)
{
    LuaWorld world;
    REQUIRE(world.Load(R"(
        Scene = { entities = {
            { tag = "mover", components = {
                rigidbody = {},
                script = function(entity, delta_time, elapsed_ms)
                    calls = (calls or 0) + 1
                    last_elapsed = elapsed_ms
                    set_velocity(entity, delta_time * 100, 7)
                end,
            } },
        } }
    )"));
    world.scripts->Update(0.5f, 1234);
    world.scripts->Update(0.25f, 1500);
    CHECK_EQ(world.GlobalInteger("calls"), 2);
    CHECK_EQ(world.GlobalInteger("last_elapsed"), 1500);
    const auto velocity = world.registry.GetEntityByTag("mover")->GetComponent<RigidBodyComponent>().velocity;
    CHECK_EQ(velocity.x, 25.0f);
    CHECK_EQ(velocity.y, 7.0f);
}

TEST(BrokenScriptReportsOnceAndOthersKeepRunning)
{
    LuaWorld world;
    CapturedErrors errors;
    REQUIRE(world.Load(R"(
        Scene = { entities = {
            { components = { script = function() error("boom") end } },
            { components = { script = function() good_calls = (good_calls or 0) + 1 end } },
        } }
    )"));
    world.scripts->Update(0.016f, 0);
    world.scripts->Update(0.016f, 16);
    world.scripts->Update(0.016f, 32);
    CHECK_EQ(world.GlobalInteger("good_calls"), 3);
    std::string text = errors.str();
    int reports = 0;
    for (std::size_t at = text.find("boom"); at != std::string::npos; at = text.find("boom", at + 1))
    {
        ++reports;
    }
    CHECK_EQ(reports, 1);
}

TEST(BindingsReadAndWriteComponents)
{
    LuaWorld world;
    REQUIRE(world.Load(R"(
        Scene = { entities = {
            { tag = "target", components = {
                transform = { position = { x = 3, y = 4 } },
                sprite = { width = 16, height = 16 },
                projectile_emitter = {},
            } },
        } }
    )"));
    REQUIRE(world.Run(R"(
        local target = get_entity_by_tag("target")
        x, y = get_position(target)
        set_position(target, 30, 40)
        set_rotation(target, 90)
        set_sprite_row(target, 2)
        set_flip(target, true)
        set_projectile_velocity(target, 1, 2)
        no_velocity = get_velocity(target)
        nobody = get_entity_by_tag("nobody")
    )"));
    CHECK_EQ(world.GlobalInteger("x"), 3);
    CHECK_EQ(world.GlobalInteger("y"), 4);
    CHECK(world.GlobalIsNil("no_velocity"));
    CHECK(world.GlobalIsNil("nobody"));
    Entity target = *world.registry.GetEntityByTag("target");
    CHECK_EQ(target.GetComponent<TransformComponent>().position.x, 30.0f);
    CHECK_EQ(target.GetComponent<TransformComponent>().rotation, 90.0);
    CHECK_EQ(target.GetComponent<SpriteComponent>().srcRect.y, 32);
    CHECK(target.GetComponent<SpriteComponent>().flip == SDL_FLIP_HORIZONTAL);
    CHECK_EQ(target.GetComponent<ProjectileEmitterComponent>().projectileVelocity.y, 2.0f);
}

TEST(ScriptsCannotReachAKilledEntityThroughAStaleHandle)
{
    LuaWorld world;
    Entity first = world.registry.CreateEntity();
    first.AddComponent<TransformComponent>(glm::vec2(1.0f, 1.0f));
    world.registry.Update();
    lua_pushinteger(world.lua, EncodeEntity(first));
    lua_setglobal(world.lua, "old");
    first.Kill();
    world.registry.Update();
    Entity reused = world.registry.CreateEntity();
    reused.AddComponent<TransformComponent>(glm::vec2(5.0f, 5.0f));
    world.registry.Update();
    REQUIRE(reused.GetId() == first.GetId());

    REQUIRE(world.Run("set_position(old, 99, 99); stale = get_position(old)"));
    CHECK_EQ(reused.GetComponent<TransformComponent>().position.x, 5.0f);
    CHECK(world.GlobalIsNil("stale"));
    CHECK(!DecodeEntity(EncodeEntity(first), world.registry).has_value());
    CHECK(DecodeEntity(EncodeEntity(reused), world.registry) == reused);
}
