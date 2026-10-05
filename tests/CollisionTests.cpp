#include "./TestFramework.h"
#include "./SDLTestHelpers.h"
#include <vector>
#include "ECS/ECS.h"
#include "EventBus/EventBus.h"
#include "Events/CollisionEvent.h"
#include "Components/TransformComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Systems/CollisionSystem.h"
#include "Systems/RenderColliderSystem.h"
#include "Systems/MovementSystem.h"
#include "Systems/CollisionResponseSystem.h"
#include "Components/SolidComponent.h"
#include "Components/CollisionResponseComponent.h"

namespace
{
    struct CollisionWorld
    {
        Registry registry;
        EventBus eventBus;
        CollisionSystem *system = &registry.AddSystem<CollisionSystem>();
        std::vector<std::pair<Entity, Entity>> hits;

        CollisionWorld()
        {
            eventBus.Subscribe<CollisionEvent>([this](CollisionEvent &event) { hits.push_back({event.a, event.b}); });
        }
        Entity Box(float x, float y, int w, int h, glm::vec2 scale = glm::vec2(1.0f, 1.0f),
                   glm::vec2 offset = glm::vec2(0.0f, 0.0f))
        {
            Entity entity = registry.CreateEntity();
            entity.AddComponent<TransformComponent>(glm::vec2(x, y), scale);
            entity.AddComponent<BoxColliderComponent>(w, h, offset);
            return entity;
        }
        void Step()
        {
            hits.clear();
            registry.Update();
            system->Update(eventBus);
        }
    };
}

TEST(CollisionSystemReportsOverlappingBoxesOnce)
{
    CollisionWorld world;
    Entity a = world.Box(0, 0, 10, 10);
    Entity b = world.Box(5, 5, 10, 10);
    world.Box(100, 100, 10, 10);
    world.Step();
    REQUIRE(world.hits.size() == 1u);
    CHECK(world.hits[0].first == a);
    CHECK(world.hits[0].second == b);
}

TEST(BoxesThatOnlyTouchDoNotCollide)
{
    CollisionWorld world;
    world.Box(0, 0, 10, 10);
    world.Box(10, 0, 10, 10);
    world.Box(0, 10, 10, 10);
    world.Step();
    CHECK(world.hits.empty());
}

TEST(ColliderSizeAndOffsetFollowTheTransformScale)
{
    CollisionWorld world;
    world.Box(0, 0, 10, 10, glm::vec2(2.0f, 2.0f));
    world.Box(19, 19, 4, 4);
    world.Step();
    CHECK_EQ(world.hits.size(), 1u);

    CollisionWorld offsetWorld;
    offsetWorld.Box(0, 0, 4, 4, glm::vec2(2.0f, 2.0f), glm::vec2(10.0f, 0.0f));
    offsetWorld.Box(2, 2, 4, 4);
    offsetWorld.Step();
    CHECK(offsetWorld.hits.empty());
}

TEST(KilledEntitiesStopCollidingImmediately)
{
    CollisionWorld world;
    Entity bullet = world.Box(0, 0, 4, 4);
    world.Box(1, 1, 4, 4);
    world.Box(2, 2, 4, 4);
    world.eventBus.Subscribe<CollisionEvent>([&](CollisionEvent &event) {
        if (event.a == bullet || event.b == bullet)
        {
            bullet.Kill();
        }
    });
    world.Step();
    int bulletHits = 0;
    for (auto &hit : world.hits)
    {
        if (hit.first == bullet || hit.second == bullet)
        {
            ++bulletHits;
        }
    }
    CHECK_EQ(bulletHits, 1);
    CHECK_EQ(world.hits.size(), 2u);
}

namespace
{
    struct BounceWorld
    {
        Registry registry;
        EventBus eventBus;
        MovementSystem *movement = &registry.AddSystem<MovementSystem>();
        CollisionSystem *collision = &registry.AddSystem<CollisionSystem>();
        CollisionResponseSystem *response = &registry.AddSystem<CollisionResponseSystem>();
        Entity enemy = registry.CreateEntity();

        BounceWorld(glm::vec2 velocity, SolidResponse onSolid = SolidResponse::Bounce)
        {
            response->SubscribeToEvents(eventBus);
            enemy.AddComponent<TransformComponent>(glm::vec2(0.0f, 0.0f));
            enemy.AddComponent<RigidBodyComponent>(velocity);
            enemy.AddComponent<SpriteComponent>("tank", 10, 10);
            enemy.AddComponent<BoxColliderComponent>(10, 10);
            enemy.AddComponent<CollisionResponseComponent>(onSolid, true);
        }
        void Wall(float x, float y)
        {
            Entity wall = registry.CreateEntity();
            wall.AddComponent<TransformComponent>(glm::vec2(x, y));
            wall.AddComponent<BoxColliderComponent>(10, 10);
            wall.AddComponent<SolidComponent>();
        }
        void Step(float deltaTime)
        {
            registry.Update();
            movement->Update(deltaTime);
            collision->Update(eventBus);
        }
        glm::vec2 Velocity() { return enemy.GetComponent<RigidBodyComponent>().velocity; }
    };
}

TEST(EnemyBouncesOffAnObstacleAndFlipsItsSprite)
{
    BounceWorld world(glm::vec2(10.0f, 0.0f));
    world.Wall(15.0f, 0.0f);
    world.Step(0.6f);
    CHECK_EQ(world.Velocity().x, -10.0f);
    CHECK(world.enemy.GetComponent<SpriteComponent>().flip == SDL_FLIP_HORIZONTAL);
}

TEST(EnemyStillOverlappingAfterBouncingDoesNotReverseAgain)
{
    BounceWorld world(glm::vec2(10.0f, 0.0f));
    world.Wall(15.0f, 0.0f);
    world.Step(1.0f);
    REQUIRE(world.Velocity().x == -10.0f);
    // Checked after every step: an even number of wrong flips would otherwise cancel out.
    for (int step = 0; step < 3; ++step)
    {
        world.Step(0.1f);
        CHECK_EQ(world.Velocity().x, -10.0f);
        CHECK(world.enemy.GetComponent<SpriteComponent>().flip == SDL_FLIP_HORIZONTAL);
    }
}

TEST(VerticalBounceDoesNotFlipTheSprite)
{
    BounceWorld world(glm::vec2(0.0f, 10.0f));
    world.Wall(0.0f, 15.0f);
    world.Step(0.6f);
    CHECK_EQ(world.Velocity().y, -10.0f);
    CHECK(world.enemy.GetComponent<SpriteComponent>().flip == SDL_FLIP_NONE);
}

TEST(EntitiesWithoutAResponsePassThroughSolids)
{
    BounceWorld world(glm::vec2(10.0f, 0.0f));
    world.enemy.RemoveComponent<CollisionResponseComponent>();
    world.Wall(15.0f, 0.0f);
    world.Step(0.6f);
    world.Step(0.6f);
    CHECK_EQ(world.Velocity().x, 10.0f);
    CHECK(world.enemy.IsAlive());
}

TEST(BounceDoesNotFlipTheSpriteUnlessAskedTo)
{
    BounceWorld world(glm::vec2(10.0f, 0.0f));
    world.enemy.GetComponent<CollisionResponseComponent>().flipSpriteOnBounce = false;
    world.Wall(15.0f, 0.0f);
    world.Step(0.6f);
    CHECK_EQ(world.Velocity().x, -10.0f);
    CHECK(world.enemy.GetComponent<SpriteComponent>().flip == SDL_FLIP_NONE);
}

TEST(NonSolidCollidersDoNotTriggerResponses)
{
    BounceWorld world(glm::vec2(10.0f, 0.0f));
    Entity ghost = world.registry.CreateEntity();
    ghost.AddComponent<TransformComponent>(glm::vec2(15.0f, 0.0f));
    ghost.AddComponent<BoxColliderComponent>(10, 10);
    world.Step(0.6f);
    CHECK_EQ(world.Velocity().x, 10.0f);
}

TEST(DestroyResponseKillsTheMoverOnASolid)
{
    BounceWorld world(glm::vec2(10.0f, 0.0f), SolidResponse::Destroy);
    world.Wall(15.0f, 0.0f);
    world.Step(0.6f);
    CHECK(!world.enemy.IsAlive());
}

TEST(BlockResponsePushesOutAlongTheShallowAxisAndStopsThatVelocity)
{
    BounceWorld world(glm::vec2(10.0f, 3.0f), SolidResponse::Block);
    world.Wall(15.0f, -20.0f);
    world.Wall(15.0f, -10.0f);
    world.Wall(15.0f, 0.0f);
    world.Wall(15.0f, 10.0f);
    world.Step(0.7f);
    const auto &transform = world.enemy.GetComponent<TransformComponent>();
    CHECK_EQ(transform.position.x, 5.0f);
    CHECK_EQ(world.Velocity().x, 0.0f);
    CHECK_EQ(world.Velocity().y, 3.0f);
}

TEST(BlockResponseKeepsVelocityThatPointsAwayFromTheSolid)
{
    BounceWorld world(glm::vec2(0.0f, 0.0f), SolidResponse::Block);
    world.Wall(8.0f, 0.0f);
    world.enemy.GetComponent<RigidBodyComponent>().velocity = glm::vec2(-5.0f, 0.0f);
    world.Step(0.0f);
    CHECK_EQ(world.enemy.GetComponent<TransformComponent>().position.x, -2.0f);
    CHECK_EQ(world.Velocity().x, -5.0f);
}

TEST(RenderColliderSystemOutlinesCollidersRelativeToTheCamera)
{
    SoftwareRenderTarget target(32, 32);
    REQUIRE(target.IsValid());
    Registry registry;
    auto &render = registry.AddSystem<RenderColliderSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<TransformComponent>(glm::vec2(104.0f, 104.0f));
    entity.AddComponent<BoxColliderComponent>(8, 8);
    registry.Update();

    render.Render(target.Renderer(), SDL_Rect{100, 100, 32, 32});
    target.Present();
    const Rgb yellow{255, 255, 0};
    CHECK_EQ(target.PixelAt(4, 4), yellow);
    CHECK_EQ(target.PixelAt(11, 11), yellow);
    CHECK_EQ(target.PixelAt(7, 4), yellow);
    CHECK_EQ(target.PixelAt(7, 7), BLACK);
}
