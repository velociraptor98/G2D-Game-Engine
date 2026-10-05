#include "./TestFramework.h"
#include "./SDLTestHelpers.h"
#include "ECS/ECS.h"
#include "Assets/AssetManager.h"
#include "Components/TransformComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "Systems/MovementSystem.h"
#include "Systems/RenderSystem.h"

TEST(MovementSystemAppliesVelocityScaledByDeltaTime)
{
    Registry registry;
    auto &movement = registry.AddSystem<MovementSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<TransformComponent>(glm::vec2(10.0f, 20.0f));
    entity.AddComponent<RigidBodyComponent>(glm::vec2(100.0f, -50.0f));
    registry.Update();

    movement.Update(0.5f);
    const auto &transform = entity.GetComponent<TransformComponent>();
    CHECK_EQ(transform.position.x, 60.0f);
    CHECK_EQ(transform.position.y, -5.0f);
}

TEST(MovementSystemIgnoresEntitiesWithoutRigidBody)
{
    Registry registry;
    auto &movement = registry.AddSystem<MovementSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<TransformComponent>(glm::vec2(10.0f, 20.0f));
    registry.Update();

    movement.Update(1.0f);
    CHECK(movement.GetEntities().empty());
    CHECK_EQ(entity.GetComponent<TransformComponent>().position.x, 10.0f);
}

TEST(RenderSystemDrawsSpriteAtPositionAndScale)
{
    SoftwareRenderTarget target(32, 32);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddTexture(target.Renderer(), "red", WriteSolidImage("red4", 4, 4, RED));
    Registry registry;
    auto &render = registry.AddSystem<RenderSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<TransformComponent>(glm::vec2(10.0f, 10.0f), glm::vec2(2.0f, 2.0f));
    entity.AddComponent<SpriteComponent>("red", 4, 4);
    registry.Update();

    render.Render(target.Renderer(), assets, SDL_Rect{0, 0, 32, 32});
    target.Present();
    CHECK_EQ(target.PixelAt(10, 10), RED);
    CHECK_EQ(target.PixelAt(17, 17), RED);
    CHECK_EQ(target.PixelAt(9, 9), BLACK);
    CHECK_EQ(target.PixelAt(18, 18), BLACK);
}

TEST(RenderSystemDrawsHigherZIndexOnTopRegardlessOfCreationOrder)
{
    SoftwareRenderTarget target(16, 16);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddTexture(target.Renderer(), "red", WriteSolidImage("red8", 8, 8, RED));
    assets.AddTexture(target.Renderer(), "blue", WriteSolidImage("blue8", 8, 8, BLUE));
    Registry registry;
    auto &render = registry.AddSystem<RenderSystem>();
    Entity top = registry.CreateEntity();
    top.AddComponent<TransformComponent>(glm::vec2(0.0f, 0.0f));
    top.AddComponent<SpriteComponent>("red", 8, 8, 2);
    Entity bottom = registry.CreateEntity();
    bottom.AddComponent<TransformComponent>(glm::vec2(0.0f, 0.0f));
    bottom.AddComponent<SpriteComponent>("blue", 8, 8, 1);
    registry.Update();

    render.Render(target.Renderer(), assets, SDL_Rect{0, 0, 32, 32});
    target.Present();
    CHECK_EQ(target.PixelAt(4, 4), RED);
}

TEST(RenderSystemDrawsOnlyTheSourceRectOfASpritesheet)
{
    SoftwareRenderTarget target(16, 16);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddTexture(target.Renderer(), "sheet", WriteTestImage("sheet", 8, 4, RED, BLUE, 4));
    Registry registry;
    auto &render = registry.AddSystem<RenderSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<TransformComponent>(glm::vec2(0.0f, 0.0f));
    entity.AddComponent<SpriteComponent>("sheet", 4, 4, 0, false, 4, 0);
    registry.Update();

    render.Render(target.Renderer(), assets, SDL_Rect{0, 0, 32, 32});
    target.Present();
    CHECK_EQ(target.PixelAt(0, 0), BLUE);
    CHECK_EQ(target.PixelAt(3, 3), BLUE);
    CHECK_EQ(target.PixelAt(4, 0), BLACK);
}

TEST(RenderSystemStopsDrawingKilledEntities)
{
    SoftwareRenderTarget target(8, 8);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddTexture(target.Renderer(), "red", WriteSolidImage("red4", 4, 4, RED));
    Registry registry;
    auto &render = registry.AddSystem<RenderSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<TransformComponent>(glm::vec2(0.0f, 0.0f));
    entity.AddComponent<SpriteComponent>("red", 4, 4);
    registry.Update();
    entity.Kill();
    registry.Update();

    render.Render(target.Renderer(), assets, SDL_Rect{0, 0, 32, 32});
    target.Present();
    CHECK_EQ(target.PixelAt(0, 0), BLACK);
}
