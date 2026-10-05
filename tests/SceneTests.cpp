#include "./TestFramework.h"
#include "./EngineTestHelpers.h"
#include <algorithm>
#include <vector>
#include "Components/TransformComponent.h"
#include "Components/TextLabelComponent.h"

namespace
{
    std::vector<std::string> ShippedScenes()
    {
        std::vector<std::string> scenes;
        for (const auto &entry : std::filesystem::directory_iterator("examples"))
        {
            if (entry.path().extension() == ".lua")
                scenes.push_back(entry.path().string());
        }
        for (const auto &entry : std::filesystem::recursive_directory_iterator("games"))
        {
            if (entry.path().extension() == ".lua")
                scenes.push_back(entry.path().string());
        }
        scenes.push_back("examples/cpp-extension/scene.lua");
        std::sort(scenes.begin(), scenes.end());
        return scenes;
    }

    // The extension example's own component; registered so its scene loads cleanly.
    void RegisterSpin(Engine &engine)
    {
        engine.GetSceneLoader().RegisterComponent("spin", [](const TableReader &fields, Entity) {
            fields.Float("degrees_per_second", 0.0f);
        });
    }
}

TEST(EveryShippedSceneLoadsAndRunsWithoutWarnings)
{
    const auto scenes = ShippedScenes();
    CHECK(scenes.size() >= 10u);
    const SDL_Keycode keys[] = {SDLK_SPACE, SDLK_c, SDLK_f, SDLK_p, SDLK_l, SDLK_l, SDLK_i, SDLK_RIGHT};
    for (const auto &scene : scenes)
    {
        HeadlessEngine h;
        REQUIRE(h.ready);
        RegisterSpin(h.engine);
        std::string errors;
        bool started = false;
        {
            CapturedErrors captured;
            started = h.engine.Start(scene);
            for (int frame = 0; started && frame < 40; ++frame)
            {
                if (frame % 5 == 0)
                    h.PressKey(keys[frame / 5]);
                h.engine.RunFrame();
            }
            errors = captured.str();
        }
        if (!started || !errors.empty() || !h.engine.IsRunning())
        {
            std::clog << "  in " << scene << std::endl;
        }
        CHECK(started);
        CHECK_EQ(errors, std::string(""));
        CHECK(h.engine.IsRunning());
    }
}

TEST(ShowcaseOpensEveryEntryAndComesBack)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    std::string errors;
    {
        CapturedErrors captured;
        REQUIRE(h.engine.Start("examples/showcase.lua"));
        const std::string showcase = h.engine.GetCurrentScene();
        for (SDL_Keycode key = SDLK_1; key <= SDLK_8; ++key)
        {
            h.PressKey(key);
            h.engine.RunFrame();
            CHECK(h.engine.GetCurrentScene() != showcase);
            h.engine.RunFrame();
            h.PressKey(SDLK_ESCAPE);
            h.engine.RunFrame();
            CHECK_EQ(h.engine.GetCurrentScene(), showcase);
        }
        h.PressKey(SDLK_ESCAPE);
        h.engine.RunFrame();
        CHECK(!h.engine.IsRunning());
        errors = captured.str();
    }
    CHECK_EQ(errors, std::string(""));
}

TEST(JungleRulesLiveInItsSceneFile)
{
    HeadlessEngine h;
    REQUIRE(h.ready);
    REQUIRE(h.engine.Start("games/jungle/level1.lua"));
    h.engine.RunFrame();
    for (auto enemy : h.engine.GetRegistry().GetEntitiesByGroup("enemies"))
    {
        enemy.Kill();
    }
    h.engine.RunFrame();
    h.engine.RunFrame();
    auto status = h.engine.GetRegistry().GetEntityByTag("status-label");
    REQUIRE(status.has_value());
    CHECK_EQ(status->GetComponent<TextLabelComponent>().text, std::string("MISSION COMPLETE"));

    h.engine.GetRegistry().GetEntityByTag("player")->Kill();
    h.engine.RunFrame();
    h.engine.RunFrame();
    CHECK_EQ(h.engine.GetRegistry().GetEntityByTag("status-label")->GetComponent<TextLabelComponent>().text,
             std::string("GAME OVER"));

    h.PressKey(SDLK_r);
    h.engine.RunFrame();
    CHECK(h.HasTag("player"));
    CHECK_EQ(h.engine.GetRegistry().GetEntitiesByGroup("enemies").size(), 6u);
}
