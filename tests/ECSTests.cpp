#include "./TestFramework.h"
#include "ECS/ECS.h"

namespace
{
    struct Position
    {
        int x;
        Position(int x = 0) : x(x) {}
    };
    struct Velocity
    {
        int v;
        Velocity(int v = 0) : v(v) {}
    };
    struct Tag
    {
    };

    class PositionVelocitySystem : public System
    {
    public:
        PositionVelocitySystem()
        {
            RequireComponent<Position>();
            RequireComponent<Velocity>();
        }
    };

    class PositionSystem : public System
    {
    public:
        PositionSystem() { RequireComponent<Position>(); }
    };

    bool Contains(const System &system, Entity entity)
    {
        for (auto e : system.GetEntities())
        {
            if (e == entity)
            {
                return true;
            }
        }
        return false;
    }
}

TEST(EntitiesJoinMatchingSystemsOnlyAfterUpdate)
{
    Registry registry;
    auto &system = registry.AddSystem<PositionVelocitySystem>();
    Entity moving = registry.CreateEntity();
    moving.AddComponent<Position>(1);
    moving.AddComponent<Velocity>(10);
    Entity still = registry.CreateEntity();
    still.AddComponent<Position>(2);

    CHECK(system.GetEntities().empty());
    registry.Update();
    CHECK_EQ(system.GetEntities().size(), 1u);
    CHECK(Contains(system, moving));
    CHECK(!Contains(system, still));
}

TEST(HasComponentReflectsAddedComponents)
{
    Registry registry;
    Entity entity = registry.CreateEntity();
    entity.AddComponent<Position>(1);
    CHECK(entity.HasComponent<Position>());
    CHECK(!entity.HasComponent<Velocity>());
}

TEST(SystemAddedAfterEntitiesPicksThemUp)
{
    Registry registry;
    Entity a = registry.CreateEntity();
    a.AddComponent<Position>(1);
    Entity b = registry.CreateEntity();
    b.AddComponent<Position>(2);
    registry.Update();

    auto &system = registry.AddSystem<PositionSystem>();
    registry.Update();
    CHECK_EQ(system.GetEntities().size(), 2u);
}

TEST(GetSystemReturnsTheRegisteredInstance)
{
    Registry registry;
    auto &added = registry.AddSystem<PositionSystem>();
    CHECK(registry.HasSystem<PositionSystem>());
    CHECK(!registry.HasSystem<PositionVelocitySystem>());
    CHECK(&registry.GetSystem<PositionSystem>() == &added);
    registry.RemoveSystem<PositionSystem>();
    CHECK(!registry.HasSystem<PositionSystem>());
}

TEST(KillIsDeferredUntilUpdate)
{
    Registry registry;
    auto &system = registry.AddSystem<PositionSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<Position>(7);
    registry.Update();

    entity.Kill();
    CHECK(Contains(system, entity));
    CHECK_EQ(entity.GetComponent<Position>().x, 7);
    registry.Update();
    CHECK(!Contains(system, entity));
}

TEST(KillingAnEntityKeepsOtherEntitiesComponentsIntact)
{
    Registry registry;
    registry.AddSystem<PositionVelocitySystem>();
    Entity a = registry.CreateEntity();
    a.AddComponent<Position>(1);
    a.AddComponent<Velocity>(10);
    Entity b = registry.CreateEntity();
    b.AddComponent<Position>(2);
    Entity c = registry.CreateEntity();
    c.AddComponent<Position>(3);
    c.AddComponent<Velocity>(30);
    registry.Update();

    a.Kill();
    registry.Update();
    // Adding new components reuses the slots freed by the kill, so a stale
    // index for a moved component would now read the newcomer's data.
    Entity d = registry.CreateEntity();
    d.AddComponent<Position>(4);
    d.AddComponent<Velocity>(40);
    CHECK_EQ(b.GetComponent<Position>().x, 2);
    CHECK_EQ(c.GetComponent<Position>().x, 3);
    CHECK_EQ(c.GetComponent<Velocity>().v, 30);
    CHECK_EQ(d.GetComponent<Position>().x, 4);
    CHECK_EQ(d.GetComponent<Velocity>().v, 40);
}

TEST(KilledIdIsReusedWithNoComponents)
{
    Registry registry;
    auto &system = registry.AddSystem<PositionSystem>();
    Entity first = registry.CreateEntity();
    first.AddComponent<Position>(1);
    registry.Update();
    first.Kill();
    registry.Update();

    Entity reused = registry.CreateEntity();
    CHECK_EQ(reused.GetId(), first.GetId());
    CHECK(!reused.HasComponent<Position>());
    registry.Update();
    CHECK(system.GetEntities().empty());
}

TEST(KillingTwiceDoesNotHandOutTheSameIdTwice)
{
    Registry registry;
    Entity entity = registry.CreateEntity();
    registry.Update();
    entity.Kill();
    entity.Kill();
    registry.Update();
    entity.Kill();
    registry.Update();

    Entity a = registry.CreateEntity();
    Entity b = registry.CreateEntity();
    CHECK(a.GetId() != b.GetId());
}

TEST(RemoveComponentIsDeferredAndThenLeavesSystem)
{
    Registry registry;
    auto &system = registry.AddSystem<PositionVelocitySystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<Position>(1);
    entity.AddComponent<Velocity>(30);
    registry.Update();

    entity.RemoveComponent<Velocity>();
    CHECK(!entity.HasComponent<Velocity>());
    CHECK(Contains(system, entity));
    CHECK_EQ(entity.GetComponent<Velocity>().v, 30);
    registry.Update();
    CHECK(!Contains(system, entity));
}

TEST(RemoveThenReAddInOneFrameKeepsTheNewValue)
{
    Registry registry;
    auto &system = registry.AddSystem<PositionVelocitySystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<Position>(1);
    entity.AddComponent<Velocity>(5);
    registry.Update();

    entity.RemoveComponent<Velocity>();
    entity.AddComponent<Velocity>(7);
    registry.Update();
    CHECK(Contains(system, entity));
    CHECK_EQ(entity.GetComponent<Velocity>().v, 7);
}

TEST(AddingAComponentTwiceReplacesIt)
{
    Registry registry;
    auto &system = registry.AddSystem<PositionSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<Position>(1);
    entity.AddComponent<Position>(9);
    registry.Update();
    CHECK_EQ(system.GetEntities().size(), 1u);
    CHECK_EQ(entity.GetComponent<Position>().x, 9);
}

TEST(ManyEntitiesKeepTheirOwnData)
{
    Registry registry;
    auto &system = registry.AddSystem<PositionSystem>();
    for (int i = 0; i < 1000; ++i)
    {
        Entity entity = registry.CreateEntity();
        entity.AddComponent<Position>(i);
        entity.AddComponent<Tag>();
        if (i % 2 == 1)
        {
            entity.Kill();
        }
    }
    registry.Update();
    REQUIRE(system.GetEntities().size() == 500u);
    for (auto entity : system.GetEntities())
    {
        CHECK_EQ(entity.GetComponent<Position>().x % 2, 0);
    }
}

TEST(StaleHandleIsNotAliveAfterItsIdIsReused)
{
    Registry registry;
    Entity first = registry.CreateEntity();
    first.AddComponent<Position>(1);
    registry.Update();
    first.Kill();
    registry.Update();
    Entity reused = registry.CreateEntity();
    reused.AddComponent<Position>(2);
    registry.Update();

    REQUIRE(reused.GetId() == first.GetId());
    CHECK(first != reused);
    CHECK(!first.IsAlive());
    CHECK(reused.IsAlive());
    CHECK(!first.HasComponent<Position>());
}

TEST(KillingAStaleHandleLeavesTheNewEntityAlone)
{
    Registry registry;
    Entity first = registry.CreateEntity();
    registry.Update();
    first.Kill();
    registry.Update();
    Entity reused = registry.CreateEntity();
    registry.Update();

    first.Kill();
    registry.Update();
    CHECK(reused.IsAlive());
}

TEST(EntityIsNotAliveOnceKillIsRequested)
{
    Registry registry;
    auto &system = registry.AddSystem<PositionSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<Position>(1);
    registry.Update();
    CHECK(entity.IsAlive());
    entity.Kill();
    CHECK(!entity.IsAlive());
    CHECK(Contains(system, entity));
}

TEST(TagsAreUniqueAndFollowTheirEntity)
{
    Registry registry;
    Entity a = registry.CreateEntity();
    Entity b = registry.CreateEntity();
    a.Tag("player");
    CHECK(a.HasTag("player"));
    REQUIRE(registry.GetEntityByTag("player").has_value());
    CHECK(*registry.GetEntityByTag("player") == a);

    b.Tag("player");
    CHECK(!a.HasTag("player"));
    CHECK(*registry.GetEntityByTag("player") == b);

    b.Kill();
    registry.Update();
    CHECK(!registry.GetEntityByTag("player").has_value());
}

TEST(GroupsCollectEntitiesAndForgetKilledOnes)
{
    Registry registry;
    Entity a = registry.CreateEntity();
    Entity b = registry.CreateEntity();
    Entity c = registry.CreateEntity();
    a.Group("enemies");
    b.Group("enemies");
    c.Group("tiles");
    CHECK(a.BelongsToGroup("enemies"));
    CHECK(!c.BelongsToGroup("enemies"));
    CHECK_EQ(registry.GetEntitiesByGroup("enemies").size(), 2u);

    a.Kill();
    registry.Update();
    CHECK_EQ(registry.GetEntitiesByGroup("enemies").size(), 1u);
    Entity reused = registry.CreateEntity();
    CHECK(!reused.BelongsToGroup("enemies"));
    CHECK(registry.GetEntitiesByGroup("missing").empty());
}

TEST(MovingAnEntityToAnotherGroupLeavesTheOldOne)
{
    Registry registry;
    Entity entity = registry.CreateEntity();
    entity.Group("enemies");
    entity.Group("wrecks");
    CHECK(registry.GetEntitiesByGroup("enemies").empty());
    CHECK(entity.BelongsToGroup("wrecks"));
}

namespace
{
    class RemovalTrackingSystem : public System
    {
    public:
        RemovalTrackingSystem() { RequireComponent<Position>(); }
        void OnEntityRemoved(Entity entity) override { lastRemovedX = entity.GetComponent<Position>().x; }
        int lastRemovedX = -1;
    };
}

TEST(SystemsAreToldWhenAnEntityLeavesWithItsComponentsStillReadable)
{
    Registry registry;
    auto &system = registry.AddSystem<RemovalTrackingSystem>();
    Entity killed = registry.CreateEntity();
    killed.AddComponent<Position>(5);
    Entity stripped = registry.CreateEntity();
    stripped.AddComponent<Position>(8);
    Entity bystander = registry.CreateEntity();
    registry.Update();

    killed.Kill();
    registry.Update();
    CHECK_EQ(system.lastRemovedX, 5);
    stripped.RemoveComponent<Position>();
    registry.Update();
    CHECK_EQ(system.lastRemovedX, 8);
    bystander.Kill();
    registry.Update();
    CHECK_EQ(system.lastRemovedX, 8);
}
