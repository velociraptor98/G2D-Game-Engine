#include "./TestFramework.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include "./EngineTestHelpers.h"
#include "Components/TransformComponent.h"
#include "Components/SpriteComponent.h"

namespace
{
    std::string WriteScene(const std::string &name, const std::string &source)
    {
        const auto dir = std::filesystem::temp_directory_path() / "g2d-engine-tests";
        std::filesystem::create_directories(dir);
        const auto path = dir / name;
        std::ofstream(path) << source;
        return path.string();
    }

    const char *SCENE_A = R"(
        Scene = {
            assets = { { type = "texture", id = "a-texture", file = "assets/images/tank-big-right.png" } },
            entities = { { tag = "from-a", components = { transform = {} } } },
            on_key = function(key)
                if key == "N" then load_scene(next_scene) return true end
                return false
            end,
        }
        a_global = 1
    )";
}

TEST(EngineLoadsASceneAndDefaultsTheWorldToTheWindow)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    REQUIRE(h.engine.Start(WriteScene("a.lua", SCENE_A)));
    CHECK(h.engine.IsRunning());
    CHECK(h.HasTag("from-a"));
    CHECK(h.engine.GetAssets().GetTexture("a-texture") != nullptr);
    CHECK_EQ(h.engine.GetSceneInfo().worldWidth, 800);
    h.engine.RunFrame();
    CHECK(h.engine.IsRunning());
}

TEST(SwitchingScenesUnloadsEntitiesAssetsAndLuaGlobals)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    const std::string b = WriteScene("b.lua", R"(Scene = { entities = { { tag = "from-b" } } })");
    REQUIRE(h.engine.Start(WriteScene("a.lua", SCENE_A)));
    lua_pushstring(h.engine.GetLua(), b.c_str());
    lua_setglobal(h.engine.GetLua(), "next_scene");
    h.PressKey(SDLK_n);
    h.engine.RunFrame();
    CHECK(h.HasTag("from-b"));
    CHECK(!h.HasTag("from-a"));
    CHECK(h.engine.GetAssets().GetTexture("a-texture") == nullptr);
    CHECK_EQ(h.Global("a_global"), 0);
    CHECK_EQ(h.engine.GetCurrentScene(), b);
}

TEST(EscapeGoesBackThenQuitsWhenNothingIsLeft)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    const std::string b = WriteScene("b.lua", R"(Scene = { entities = { { tag = "from-b" } } })");
    REQUIRE(h.engine.Start(WriteScene("a.lua", SCENE_A)));
    lua_pushstring(h.engine.GetLua(), b.c_str());
    lua_setglobal(h.engine.GetLua(), "next_scene");
    h.PressKey(SDLK_n);
    h.engine.RunFrame();
    REQUIRE(h.HasTag("from-b"));
    h.PressKey(SDLK_ESCAPE);
    h.engine.RunFrame();
    CHECK(h.HasTag("from-a"));
    CHECK(h.engine.IsRunning());
    h.PressKey(SDLK_ESCAPE);
    h.engine.RunFrame();
    CHECK(!h.engine.IsRunning());
}

TEST(OnKeyCanConsumeEscape)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    REQUIRE(h.engine.Start(WriteScene("keys.lua", R"(
        Scene = { on_key = function(key) last_key = key; return key == "Escape" end }
    )")));
    h.PressKey(SDLK_ESCAPE);
    h.engine.RunFrame();
    CHECK(h.engine.IsRunning());
    lua_getglobal(h.engine.GetLua(), "last_key");
    CHECK_EQ(std::string(lua_tostring(h.engine.GetLua(), -1)), std::string("Escape"));
    lua_pop(h.engine.GetLua(), 1);
}

TEST(OnStartRunsOnceAfterTheSceneIsBuilt)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    REQUIRE(h.engine.Start(WriteScene("start.lua", R"(
        Scene = {
            entities = { { tag = "existing" } },
            on_start = function()
                starts = (starts or 0) + 1
                saw_entity = get_entity_by_tag("existing") and 1 or 0
                set_debug_overlay(true)
            end,
        }
    )")));
    h.engine.RunFrame();
    h.engine.RunFrame();
    CHECK_EQ(h.Global("starts"), 1);
    CHECK_EQ(h.Global("saw_entity"), 1);
    CHECK(h.engine.IsDebugOverlay());
}

TEST(OnUpdateRunsEveryFrame)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    REQUIRE(h.engine.Start(WriteScene("update.lua", R"(
        Scene = { on_update = function(dt, elapsed) frames = (frames or 0) + 1 end }
    )")));
    h.engine.RunFrame();
    h.engine.RunFrame();
    h.engine.RunFrame();
    CHECK_EQ(h.Global("frames"), 3);
}

TEST(ReloadRestartsTheSceneWithoutAddingHistory)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    REQUIRE(h.engine.Start(WriteScene("reload.lua", R"(
        Scene = {
            entities = { { tag = "thing" } },
            on_key = function(key) if key == "R" then reload_scene() return true end end,
        }
    )")));
    lua_pushinteger(h.engine.GetLua(), 42);
    lua_setglobal(h.engine.GetLua(), "marker");
    h.PressKey(SDLK_r);
    h.engine.RunFrame();
    CHECK(h.HasTag("thing"));
    CHECK_EQ(h.Global("marker"), 0);
    h.PressKey(SDLK_ESCAPE);
    h.engine.RunFrame();
    CHECK(!h.engine.IsRunning());
}

TEST(AFailingSceneFallsBackToTheOneThatWasRunning)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    REQUIRE(h.engine.Start(WriteScene("a.lua", SCENE_A)));
    lua_pushstring(h.engine.GetLua(), WriteScene("broken.lua", "Scene = {").c_str());
    lua_setglobal(h.engine.GetLua(), "next_scene");
    CapturedErrors errors;
    h.PressKey(SDLK_n);
    h.engine.RunFrame();
    CHECK(h.engine.IsRunning());
    CHECK(h.HasTag("from-a"));
    CHECK(errors.str().find("broken.lua") != std::string::npos);
}

TEST(SpawnCreatesAnEntityFromADefinition)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    REQUIRE(h.engine.Start(WriteScene("spawn.lua", R"(
        Scene = { on_key = function(key)
            local spawned = spawn({ tag = "spawned", components = { transform = { position = { x = 7, y = 9 } } } })
            spawned_alive = is_alive(spawned) and 1 or 0
            return true
        end }
    )")));
    h.PressKey(SDLK_s);
    h.engine.RunFrame();
    auto spawned = h.engine.GetRegistry().GetEntityByTag("spawned");
    REQUIRE(spawned.has_value());
    CHECK_EQ(spawned->GetComponent<TransformComponent>().position.y, 9.0f);
    CHECK_EQ(h.Global("spawned_alive"), 1);
}

TEST(DebugOverlayIsSetFromLuaAndResetBetweenScenes)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    const std::string scene = WriteScene("debug.lua", R"(
        Scene = { on_key = function(key)
            if key == "C" then toggle_debug_overlay() elseif key == "R" then reload_scene() end
            return true
        end }
    )");
    REQUIRE(h.engine.Start(scene));
    h.PressKey(SDLK_c);
    h.engine.RunFrame();
    CHECK(h.engine.IsDebugOverlay());
    h.PressKey(SDLK_r);
    h.engine.RunFrame();
    CHECK(!h.engine.IsDebugOverlay());
}

namespace
{
    struct SpinComponent
    {
        float degreesPerSecond;
        SpinComponent(float degreesPerSecond = 0.0f) : degreesPerSecond(degreesPerSecond) {}
    };

    class SpinSystem : public System
    {
    public:
        SpinSystem()
        {
            RequireComponent<TransformComponent>();
            RequireComponent<SpinComponent>();
        }
        void Update(float deltaTime)
        {
            for (auto entity : GetEntities())
            {
                entity.GetComponent<TransformComponent>().rotation +=
                    entity.GetComponent<SpinComponent>().degreesPerSecond * deltaTime;
            }
        }
    };
}

TEST(GamesCanAddComponentsSystemsAndHooks)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    h.engine.GetSceneLoader().RegisterComponent("spin", [](const TableReader &fields, Entity entity) {
        entity.AddComponent<SpinComponent>(fields.Float("degrees_per_second", 0.0f));
    });
    auto &spin = h.engine.GetRegistry().AddSystem<SpinSystem>();
    int renders = 0;
    h.engine.AddUpdateHook([&](float) { spin.Update(1.0f); });
    h.engine.AddRenderHook([&](SDL_Renderer *renderer) { renders += renderer != nullptr; });
    CapturedErrors errors;
    REQUIRE(h.engine.Start(WriteScene("spin.lua", R"(
        Scene = { entities = { { tag = "spinner", components = { transform = {}, spin = { degrees_per_second = 90 } } } } }
    )")));
    h.engine.RunFrame();
    h.engine.RunFrame();
    CHECK_EQ(errors.str(), std::string(""));
    CHECK_EQ(h.engine.GetRegistry().GetEntityByTag("spinner")->GetComponent<TransformComponent>().rotation, 180.0);
    CHECK_EQ(renders, 2);
}

TEST(ABrokenOnUpdateReportsOnceAndTheSceneKeepsRunning)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    REQUIRE(h.engine.Start(WriteScene("bad-update.lua", R"(Scene = { on_update = function() error("kaput") end })")));
    CapturedErrors errors;
    h.engine.RunFrame();
    h.engine.RunFrame();
    CHECK(h.engine.IsRunning());
    const std::string text = errors.str();
    CHECK(text.find("kaput") != std::string::npos);
    CHECK_EQ(text.find("kaput"), text.rfind("kaput"));
}

TEST(RelativePathsResolveAgainstTheEngineRoot)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    const std::string resolved = h.engine.ResolvePath("assets/images/radar.png");
    CHECK(std::filesystem::path(resolved).is_absolute());
    CHECK(std::filesystem::exists(resolved));
    CHECK_EQ(h.engine.ResolvePath("/already/absolute"), std::string("/already/absolute"));
}
