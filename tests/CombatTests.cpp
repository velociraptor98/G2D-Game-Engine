#include "./TestFramework.h"
#include "ECS/ECS.h"
#include "EventBus/EventBus.h"
#include "Events/KeyPressedEvent.h"
#include "Components/TransformComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Components/HealthComponent.h"
#include "Components/ProjectileComponent.h"
#include "Components/ProjectileEmitterComponent.h"
#include "Components/KeyboardControlledComponent.h"
#include "Systems/ProjectileEmitSystem.h"
#include "Systems/ProjectileLifecycleSystem.h"
#include "Systems/DamageSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/MovementSystem.h"

namespace
{
    struct CombatWorld
    {
        Registry registry;
        EventBus eventBus;
        ProjectileEmitSystem *emitter = &registry.AddSystem<ProjectileEmitSystem>(registry);
        ProjectileLifecycleSystem *lifecycle = &registry.AddSystem<ProjectileLifecycleSystem>();
        CollisionSystem *collision = &registry.AddSystem<CollisionSystem>();
        DamageSystem *damage = &registry.AddSystem<DamageSystem>();

        CombatWorld()
        {
            emitter->SubscribeToEvents(eventBus);
            damage->SubscribeToEvents(eventBus);
        }
        std::vector<Entity> Projectiles() { return registry.GetEntitiesByGroup("projectiles"); }

        Entity Turret(glm::vec2 position, int repeatFrequency)
        {
            Entity turret = registry.CreateEntity();
            turret.AddComponent<TransformComponent>(position, glm::vec2(2.0f, 2.0f));
            turret.AddComponent<SpriteComponent>("tank", 32, 32);
            turret.AddComponent<ProjectileEmitterComponent>(glm::vec2(0.0f, -100.0f), repeatFrequency, 1000, 10,
                                                            false, "bullet");
            return turret;
        }
        Entity Target(const std::string &group, glm::vec2 position, int health)
        {
            Entity target = registry.CreateEntity();
            target.AddComponent<TransformComponent>(position);
            target.AddComponent<BoxColliderComponent>(10, 10);
            target.AddComponent<HealthComponent>(health);
            if (group == "player")
            {
                target.Tag("player");
            }
            else
            {
                target.Group(group);
            }
            return target;
        }
        Entity Bullet(glm::vec2 position, bool isFriendly, int damage)
        {
            Entity bullet = registry.CreateEntity();
            bullet.AddComponent<TransformComponent>(position);
            bullet.AddComponent<BoxColliderComponent>(4, 4);
            bullet.AddComponent<ProjectileComponent>(isFriendly, damage, 1000, 0);
            bullet.Group("projectiles");
            return bullet;
        }
        void Collide()
        {
            registry.Update();
            collision->Update(eventBus);
            registry.Update();
        }
    };
}

TEST(EmitterFiresOnItsRepeatFrequency)
{
    CombatWorld world;
    world.Turret(glm::vec2(0.0f, 0.0f), 1000);
    world.registry.Update();
    world.emitter->Update(1000);
    world.emitter->Update(1500);
    world.registry.Update();
    CHECK_EQ(world.Projectiles().size(), 1u);
    world.emitter->Update(2000);
    world.registry.Update();
    CHECK_EQ(world.Projectiles().size(), 2u);
}

TEST(ProjectileStartsCentredOnItsSourceWithTheEmitterSettings)
{
    CombatWorld world;
    world.Turret(glm::vec2(100.0f, 200.0f), 1000);
    world.registry.Update();
    world.emitter->Update(1000);
    world.registry.Update();
    REQUIRE(world.Projectiles().size() == 1u);
    Entity bullet = world.Projectiles()[0];
    CHECK_EQ(bullet.GetComponent<TransformComponent>().position.x, 128.0f);
    CHECK_EQ(bullet.GetComponent<TransformComponent>().position.y, 228.0f);
    CHECK_EQ(bullet.GetComponent<RigidBodyComponent>().velocity.y, -100.0f);
    CHECK_EQ(bullet.GetComponent<SpriteComponent>().assetId, std::string("bullet"));
    CHECK(!bullet.GetComponent<ProjectileComponent>().isFriendly);
    CHECK_EQ(bullet.GetComponent<ProjectileComponent>().startTime, 1000u);
}

TEST(OnDemandEmitterIgnoresTheTimerAndFiresAlongFacingOnSpace)
{
    CombatWorld world;
    Entity player = world.Turret(glm::vec2(0.0f, 0.0f), 0);
    player.AddComponent<KeyboardControlledComponent>(100.0f, glm::vec2(0.0f, 1.0f));
    world.registry.Update();
    world.emitter->Update(50000);
    world.registry.Update();
    CHECK(world.Projectiles().empty());

    world.eventBus.Emit<KeyPressedEvent>(SDLK_SPACE);
    world.eventBus.Emit<KeyPressedEvent>(SDLK_a);
    world.registry.Update();
    REQUIRE(world.Projectiles().size() == 1u);
    const glm::vec2 velocity = world.Projectiles()[0].GetComponent<RigidBodyComponent>().velocity;
    CHECK_EQ(velocity.x, 0.0f);
    CHECK_EQ(velocity.y, 100.0f);
}

TEST(ProjectileIsKilledWhenItsDurationRunsOut)
{
    CombatWorld world;
    Entity bullet = world.Bullet(glm::vec2(0.0f, 0.0f), true, 10);
    world.registry.Update();
    world.lifecycle->Update(1000);
    CHECK(bullet.IsAlive());
    world.lifecycle->Update(1001);
    CHECK(!bullet.IsAlive());
}

TEST(FriendlyProjectileDamagesEnemyAndIsConsumed)
{
    CombatWorld world;
    Entity enemy = world.Target("enemies", glm::vec2(0.0f, 0.0f), 100);
    Entity bullet = world.Bullet(glm::vec2(2.0f, 2.0f), true, 25);
    world.Collide();
    CHECK_EQ(enemy.GetComponent<HealthComponent>().healthPercentage, 75);
    CHECK(!bullet.IsAlive());
    CHECK(enemy.IsAlive());
}

TEST(EnemyDiesWhenHealthReachesZero)
{
    CombatWorld world;
    Entity enemy = world.Target("enemies", glm::vec2(0.0f, 0.0f), 20);
    world.Bullet(glm::vec2(2.0f, 2.0f), true, 20);
    world.Collide();
    CHECK(!enemy.IsAlive());
}

TEST(FriendlyFireDoesNotHurtThePlayerAndEnemyFireDoesNotHurtEnemies)
{
    CombatWorld world;
    Entity player = world.Target("player", glm::vec2(0.0f, 0.0f), 100);
    Entity friendly = world.Bullet(glm::vec2(2.0f, 2.0f), true, 10);
    Entity enemy = world.Target("enemies", glm::vec2(100.0f, 0.0f), 100);
    Entity hostile = world.Bullet(glm::vec2(102.0f, 2.0f), false, 10);
    world.Collide();
    CHECK_EQ(player.GetComponent<HealthComponent>().healthPercentage, 100);
    CHECK_EQ(enemy.GetComponent<HealthComponent>().healthPercentage, 100);
    CHECK(friendly.IsAlive());
    CHECK(hostile.IsAlive());
}

TEST(EnemyProjectileDamagesThePlayer)
{
    CombatWorld world;
    Entity player = world.Target("player", glm::vec2(0.0f, 0.0f), 100);
    Entity bullet = world.Bullet(glm::vec2(2.0f, 2.0f), false, 10);
    world.Collide();
    CHECK_EQ(player.GetComponent<HealthComponent>().healthPercentage, 90);
    CHECK(!bullet.IsAlive());
}

TEST(ObstaclesAbsorbProjectiles)
{
    CombatWorld world;
    Entity wall = world.registry.CreateEntity();
    wall.AddComponent<TransformComponent>(glm::vec2(0.0f, 0.0f));
    wall.AddComponent<BoxColliderComponent>(10, 10);
    wall.Group("obstacles");
    Entity bullet = world.Bullet(glm::vec2(2.0f, 2.0f), true, 10);
    world.Collide();
    CHECK(!bullet.IsAlive());
    CHECK(wall.IsAlive());
}

TEST(OneProjectileOverlappingTwoEnemiesOnlyHitsOne)
{
    CombatWorld world;
    Entity first = world.Target("enemies", glm::vec2(0.0f, 0.0f), 100);
    Entity second = world.Target("enemies", glm::vec2(3.0f, 3.0f), 100);
    world.Bullet(glm::vec2(4.0f, 4.0f), true, 10);
    world.Collide();
    const int total = first.GetComponent<HealthComponent>().healthPercentage +
                      second.GetComponent<HealthComponent>().healthPercentage;
    CHECK_EQ(total, 190);
}
