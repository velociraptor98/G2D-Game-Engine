#include "./TestFramework.h"
#include "./SDLTestHelpers.h"
#include <cstdlib>
#include "ECS/ECS.h"
#include "AssetManager.h"
#include "GameRules.h"
#include "Components/TransformComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/HealthComponent.h"
#include "Components/TextLabelComponent.h"
#include "Systems/RenderTextSystem.h"
#include "Systems/RenderHealthBarSystem.h"

namespace
{
    int LitPixelsIn(const SoftwareRenderTarget &target, int x0, int y0, int x1, int y1)
    {
        int lit = 0;
        for (int y = y0; y < y1; ++y)
        {
            for (int x = x0; x < x1; ++x)
            {
                if (!(target.PixelAt(x, y) == BLACK))
                {
                    ++lit;
                }
            }
        }
        return lit;
    }
}

TEST(AssetManagerLoadsFontsAndRejectsMissingOnes)
{
    AssetManager assets;
    assets.AddFont("arial", "./assets/fonts/arial.ttf", 16);
    CHECK(assets.GetFont("arial") != nullptr);
    std::cerr << "  (a font load error is expected below)" << std::endl;
    assets.AddFont("missing", "./does/not/exist.ttf", 16);
    CHECK(assets.GetFont("missing") == nullptr);
    assets.ClearAssets();
    CHECK(assets.GetFont("arial") == nullptr);
}

TEST(RenderTextSystemDrawsLabelsWhereTheyArePlaced)
{
    SoftwareRenderTarget target(200, 100);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddFont("arial", "./assets/fonts/arial.ttf", 20);
    Registry registry;
    auto &render = registry.AddSystem<RenderTextSystem>();
    Entity label = registry.CreateEntity();
    label.AddComponent<TextLabelComponent>(glm::vec2(10.0f, 10.0f), "HELLO", "arial");
    registry.Update();

    render.Render(target.Renderer(), assets, SDL_Rect{500, 500, 200, 100});
    target.Present();
    CHECK(LitPixelsIn(target, 10, 10, 100, 40) > 20);
    CHECK_EQ(LitPixelsIn(target, 0, 50, 200, 100), 0);
}

TEST(WorldSpaceLabelsMoveWithTheCamera)
{
    SoftwareRenderTarget target(200, 100);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddFont("arial", "./assets/fonts/arial.ttf", 20);
    Registry registry;
    auto &render = registry.AddSystem<RenderTextSystem>();
    Entity label = registry.CreateEntity();
    label.AddComponent<TextLabelComponent>(glm::vec2(110.0f, 160.0f), "HELLO", "arial",
                                           SDL_Color{255, 255, 255, 255}, false);
    registry.Update();

    render.Render(target.Renderer(), assets, SDL_Rect{100, 100, 200, 100});
    target.Present();
    CHECK(LitPixelsIn(target, 10, 60, 100, 90) > 20);
    CHECK_EQ(LitPixelsIn(target, 0, 0, 200, 55), 0);
}

TEST(CentredLabelsAreCentredOnTheirPosition)
{
    SoftwareRenderTarget target(200, 100);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddFont("arial", "./assets/fonts/arial.ttf", 20);
    Registry registry;
    auto &render = registry.AddSystem<RenderTextSystem>();
    Entity label = registry.CreateEntity();
    label.AddComponent<TextLabelComponent>(glm::vec2(100.0f, 50.0f), "MMMM", "arial",
                                           SDL_Color{255, 255, 255, 255}, true, true);
    registry.Update();

    render.Render(target.Renderer(), assets, SDL_Rect{0, 0, 200, 100});
    target.Present();
    const int left = LitPixelsIn(target, 0, 0, 100, 100);
    const int right = LitPixelsIn(target, 100, 0, 200, 100);
    CHECK(left > 20 && right > 20);
    CHECK(std::abs(left - right) * 4 < left + right);
}

TEST(EmptyOrUnknownFontLabelsAreSkipped)
{
    SoftwareRenderTarget target(50, 50);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddFont("arial", "./assets/fonts/arial.ttf", 20);
    Registry registry;
    auto &render = registry.AddSystem<RenderTextSystem>();
    registry.CreateEntity().AddComponent<TextLabelComponent>(glm::vec2(0.0f, 0.0f), "", "arial");
    registry.CreateEntity().AddComponent<TextLabelComponent>(glm::vec2(0.0f, 0.0f), "X", "nope");
    registry.Update();
    render.Render(target.Renderer(), assets, SDL_Rect{0, 0, 50, 50});
    target.Present();
    CHECK_EQ(LitPixelsIn(target, 0, 0, 50, 50), 0);
}

TEST(HealthBarWidthAndColourFollowHealth)
{
    SoftwareRenderTarget target(64, 64);
    REQUIRE(target.IsValid());
    Registry registry;
    auto &render = registry.AddSystem<RenderHealthBarSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<TransformComponent>(glm::vec2(10.0f, 10.0f), glm::vec2(2.0f, 2.0f));
    entity.AddComponent<SpriteComponent>("tank", 16, 8);
    entity.AddComponent<HealthComponent>(50);
    registry.Update();

    render.Render(target.Renderer(), SDL_Rect{0, 0, 64, 64});
    target.Present();
    const int barY = 10 + 16 + RenderHealthBarSystem::BAR_GAP;
    const Rgb yellow{230, 200, 0};
    CHECK_EQ(target.PixelAt(10, barY), yellow);
    CHECK_EQ(target.PixelAt(25, barY), yellow);
    CHECK_EQ(target.PixelAt(26, barY), BLACK);
    CHECK_EQ(target.PixelAt(10, barY - 1), BLACK);
}

TEST(HealthBarColourBands)
{
    CHECK_EQ(static_cast<int>(HealthBarColor(100).g), 200);
    CHECK_EQ(static_cast<int>(HealthBarColor(71).r), 0);
    CHECK_EQ(static_cast<int>(HealthBarColor(70).r), 230);
    CHECK_EQ(static_cast<int>(HealthBarColor(41).r), 230);
    CHECK_EQ(static_cast<int>(HealthBarColor(40).g), 0);
}

TEST(MissionStatusTracksPlayerAndEnemies)
{
    Registry registry;
    CHECK_EQ(MissionStatus(registry), std::string("GAME OVER"));
    Entity player = registry.CreateEntity();
    player.Tag("player");
    CHECK_EQ(MissionStatus(registry), std::string("MISSION COMPLETE"));
    Entity enemy = registry.CreateEntity();
    enemy.Group("enemies");
    CHECK_EQ(MissionStatus(registry), std::string(""));
    enemy.Kill();
    CHECK_EQ(MissionStatus(registry), std::string("MISSION COMPLETE"));
    player.Kill();
    CHECK_EQ(MissionStatus(registry), std::string("GAME OVER"));
    registry.Update();
    CHECK_EQ(MissionStatus(registry), std::string("GAME OVER"));
}
