#include "./TestFramework.h"
#include "./SDLTestHelpers.h"
#include <fstream>
#include <sstream>
#include "Scene/TileMap.h"
#include "Components/TransformComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/CameraFollowComponent.h"
#include "Systems/CameraMovementSystem.h"
#include "Systems/RenderSystem.h"

namespace
{
    bool Parse(const std::string &text, TileMap &map, std::string &error)
    {
        std::istringstream input(text);
        return ParseTileMap(input, map, error);
    }
}

TEST(ParseTileMapReadsTileCodesAsRowThenColumn)
{
    TileMap map;
    std::string error;
    REQUIRE(Parse("00,21,09\n10,26,05\n", map, error));
    CHECK_EQ(map.columns, 3);
    CHECK_EQ(map.rows, 2);
    CHECK_EQ(map.TileAt(1, 0).x, 1);
    CHECK_EQ(map.TileAt(1, 0).y, 2);
    CHECK_EQ(map.TileAt(2, 0).x, 9);
    CHECK_EQ(map.TileAt(2, 0).y, 0);
    CHECK_EQ(map.TileAt(1, 1).x, 6);
    CHECK_EQ(map.TileAt(1, 1).y, 2);
    CHECK(!map.IsObstacle(0, 0));
}

TEST(ParseTileMapReadsTheObstacleGridAfterABlankLine)
{
    TileMap map;
    std::string error;
    REQUIRE(Parse("00,01\r\n02,03\r\n\r\n1,0\r\n0,1\r\n", map, error));
    CHECK(map.IsObstacle(0, 0));
    CHECK(!map.IsObstacle(1, 0));
    CHECK(!map.IsObstacle(0, 1));
    CHECK(map.IsObstacle(1, 1));
}

TEST(ParseTileMapRejectsRaggedRows)
{
    TileMap map;
    std::string error;
    CHECK(!Parse("00,01\n02\n", map, error));
    CHECK(error.find("line 2") != std::string::npos);
}

TEST(ParseTileMapRejectsBadCodes)
{
    TileMap map;
    std::string error;
    CHECK(!Parse("00,x1\n", map, error));
    CHECK(!Parse("00,1\n", map, error));
    CHECK(!Parse("00,01\n\n1,2\n", map, error));
}

TEST(ParseTileMapRejectsMismatchedObstacleGrid)
{
    TileMap map;
    std::string error;
    CHECK(!Parse("00,01\n02,03\n\n1,0\n", map, error));
}

TEST(TheShippedJungleMapParses)
{
    std::ifstream file("./assets/tilemaps/jungle.map");
    REQUIRE(file.good());
    TileMap map;
    std::string error;
    const bool parsed = ParseTileMap(file, map, error);
    if (!parsed)
    {
        std::cerr << "  " << error << std::endl;
    }
    REQUIRE(parsed);
    CHECK_EQ(map.columns, 25);
    CHECK_EQ(map.rows, 20);
    CHECK(map.IsObstacle(0, 0));
    CHECK(!map.IsObstacle(1, 1));
    for (const auto &tile : map.tiles)
    {
        CHECK(tile.x >= 0 && tile.x < 10 && tile.y >= 0 && tile.y < 3);
    }
}

TEST(CreateTileEntitiesPlacesScaledTilesWithTheirSourceRects)
{
    TileMap map;
    std::string error;
    REQUIRE(Parse("00,21\n13,02\n", map, error));
    Registry registry;
    auto &render = registry.AddSystem<RenderSystem>();
    CreateTileEntities(map, "tiles", 32, 2.0f, registry);
    registry.Update();
    REQUIRE(render.GetEntities().size() == 4u);
    Entity last = render.GetEntities()[3];
    CHECK_EQ(last.GetComponent<TransformComponent>().position.x, 64.0f);
    CHECK_EQ(last.GetComponent<TransformComponent>().position.y, 64.0f);
    CHECK_EQ(last.GetComponent<SpriteComponent>().srcRect.x, 64);
    CHECK_EQ(last.GetComponent<SpriteComponent>().srcRect.y, 0);
    Entity second = render.GetEntities()[1];
    CHECK_EQ(second.GetComponent<SpriteComponent>().srcRect.x, 32);
    CHECK_EQ(second.GetComponent<SpriteComponent>().srcRect.y, 64);
}

TEST(CameraCentresOnTheFollowedEntity)
{
    Registry registry;
    auto &cameraSystem = registry.AddSystem<CameraMovementSystem>();
    Entity player = registry.CreateEntity();
    player.AddComponent<TransformComponent>(glm::vec2(1000.0f, 700.0f));
    player.AddComponent<CameraFollowComponent>();
    registry.Update();

    SDL_Rect camera{0, 0, 800, 600};
    cameraSystem.Update(camera, 1600, 1280);
    CHECK_EQ(camera.x, 600);
    CHECK_EQ(camera.y, 400);
}

TEST(CameraIsClampedToTheMapEdges)
{
    Registry registry;
    auto &cameraSystem = registry.AddSystem<CameraMovementSystem>();
    Entity player = registry.CreateEntity();
    player.AddComponent<TransformComponent>(glm::vec2(10.0f, 10.0f));
    player.AddComponent<CameraFollowComponent>();
    registry.Update();

    SDL_Rect camera{0, 0, 800, 600};
    cameraSystem.Update(camera, 1600, 1280);
    CHECK_EQ(camera.x, 0);
    CHECK_EQ(camera.y, 0);
    player.GetComponent<TransformComponent>().position = glm::vec2(1590.0f, 1270.0f);
    cameraSystem.Update(camera, 1600, 1280);
    CHECK_EQ(camera.x, 800);
    CHECK_EQ(camera.y, 680);
}

TEST(CameraStaysPutWhenTheMapIsSmallerThanTheScreen)
{
    Registry registry;
    auto &cameraSystem = registry.AddSystem<CameraMovementSystem>();
    Entity player = registry.CreateEntity();
    player.AddComponent<TransformComponent>(glm::vec2(300.0f, 200.0f));
    player.AddComponent<CameraFollowComponent>();
    registry.Update();

    SDL_Rect camera{0, 0, 800, 600};
    cameraSystem.Update(camera, 400, 300);
    CHECK_EQ(camera.x, 0);
    CHECK_EQ(camera.y, 0);
}

TEST(RenderSystemOffsetsWorldSpritesByTheCameraButNotFixedOnes)
{
    SoftwareRenderTarget target(32, 32);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddTexture(target.Renderer(), "red", WriteSolidImage("red4", 4, 4, RED));
    assets.AddTexture(target.Renderer(), "blue", WriteSolidImage("blue4", 4, 4, BLUE));
    Registry registry;
    auto &render = registry.AddSystem<RenderSystem>();
    Entity world = registry.CreateEntity();
    world.AddComponent<TransformComponent>(glm::vec2(110.0f, 120.0f));
    world.AddComponent<SpriteComponent>("red", 4, 4);
    Entity hud = registry.CreateEntity();
    hud.AddComponent<TransformComponent>(glm::vec2(0.0f, 0.0f));
    hud.AddComponent<SpriteComponent>("blue", 4, 4, 0, true);
    registry.Update();

    render.Render(target.Renderer(), assets, SDL_Rect{100, 100, 32, 32});
    target.Present();
    CHECK_EQ(target.PixelAt(10, 20), RED);
    CHECK_EQ(target.PixelAt(0, 0), BLUE);
}
