#include "./TestFramework.h"
#include "ECS/ECS.h"
#include "EventBus/EventBus.h"
#include "Events/KeyPressedEvent.h"
#include "Components/TransformComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Components/HealthComponent.h"
#include "Components/TeamComponent.h"
#include "Components/DamageOnContactComponent.h"
#include "Components/LifetimeComponent.h"
#include "Components/SolidComponent.h"
#include "Components/CollisionResponseComponent.h"
#include "Components/ProjectileEmitterComponent.h"
#include "Components/KeyboardControlledComponent.h"
#include "Systems/ProjectileEmitSystem.h"
#include "Systems/LifetimeSystem.h"
#include "Systems/DamageSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/CollisionResponseSystem.h"

namespace
{
    // Counts live entities that have a given component.
    template <typename T>
    class CountingSystem : public System
    {
    public:
        CountingSystem() { RequireComponent<T>(); }
    };

    struct CombatWorld
    {
        Registry registry;
        EventBus eventBus;
        ProjectileEmitSystem *emitter = &registry.AddSystem<ProjectileEmitSystem>(registry);
        LifetimeSystem *lifetime = &registry.AddSystem<LifetimeSystem>();
        CollisionSystem *collision = &registry.AddSystem<CollisionSystem>();
        CollisionResponseSystem *response = &registry.AddSystem<CollisionResponseSystem>();
        DamageSystem *damage = &registry.AddSystem<DamageSystem>();
        CountingSystem<DamageOnContactComponent> *projectiles =
            &registry.AddSystem<CountingSystem<DamageOnContactComponent>>();

        CombatWorld()
        {
            emitter->SubscribeToEvents(eventBus);
            response->SubscribeToEvents(eventBus);
            damage->SubscribeToEvents(eventBus);
        }
        const std::vector<Entity> &Projectiles() { return projectiles->GetEntities(); }

        Entity Shooter(glm::vec2 position, ProjectileEmitterComponent emitter, const std::string &team = "")
        {
            Entity shooter = registry.CreateEntity();
            shooter.AddComponent<TransformComponent>(position, glm::vec2(2.0f, 2.0f));
            shooter.AddComponent<SpriteComponent>("tank", 32, 32);
            shooter.AddComponent<ProjectileEmitterComponent>(emitter);
            if (!team.empty())
                shooter.AddComponent<TeamComponent>(team);
            return shooter;
        }
        Entity Target(glm::vec2 position, int health, const std::string &team = "")
        {
            Entity target = registry.CreateEntity();
            target.AddComponent<TransformComponent>(position);
            target.AddComponent<BoxColliderComponent>(10, 10);
            target.AddComponent<HealthComponent>(health, health);
            if (!team.empty())
                target.AddComponent<TeamComponent>(team);
            return target;
        }
        Entity Damager(glm::vec2 position, int amount, bool destroyOnContact, const std::string &team = "")
        {
            Entity damager = registry.CreateEntity();
            damager.AddComponent<TransformComponent>(position);
            damager.AddComponent<BoxColliderComponent>(4, 4);
            damager.AddComponent<DamageOnContactComponent>(amount, destroyOnContact);
            if (!team.empty())
                damager.AddComponent<TeamComponent>(team);
            return damager;
        }
        void Collide()
        {
            registry.Update();
            collision->Update(eventBus);
            registry.Update();
        }
    };

    ProjectileTemplate Shot(int damage = 10, int lifetimeMs = 1000)
    {
        ProjectileTemplate shot;
        shot.assetId = "bullet";
        shot.damage = damage;
        shot.lifetimeMs = lifetimeMs;
        return shot;
    }
}

TEST(EmitterFiresOnItsRepeatFrequency)
{
    CombatWorld world;
    world.Shooter(glm::vec2(0.0f, 0.0f), ProjectileEmitterComponent(glm::vec2(0.0f, -100.0f), 1000));
    world.registry.Update();
    world.emitter->Update(1000);
    world.emitter->Update(1500);
    world.registry.Update();
    CHECK_EQ(world.Projectiles().size(), 1u);
    world.emitter->Update(2000);
    world.registry.Update();
    CHECK_EQ(world.Projectiles().size(), 2u);
}

TEST(ProjectileIsBuiltFromTheTemplateAndCentredOnItsShooter)
{
    CombatWorld world;
    world.Shooter(glm::vec2(100.0f, 200.0f),
                  ProjectileEmitterComponent(glm::vec2(0.0f, -100.0f), 1000, SDLK_UNKNOWN, false, Shot(7, 500)),
                  "enemies");
    world.registry.Update();
    world.emitter->Update(1000);
    world.registry.Update();
    REQUIRE(world.Projectiles().size() == 1u);
    Entity bullet = world.Projectiles()[0];
    CHECK_EQ(bullet.GetComponent<TransformComponent>().position.x, 128.0f);
    CHECK_EQ(bullet.GetComponent<TransformComponent>().position.y, 228.0f);
    CHECK_EQ(bullet.GetComponent<RigidBodyComponent>().velocity.y, -100.0f);
    CHECK_EQ(bullet.GetComponent<SpriteComponent>().assetId, std::string("bullet"));
    CHECK_EQ(bullet.GetComponent<DamageOnContactComponent>().damage, 7);
    CHECK(bullet.GetComponent<DamageOnContactComponent>().destroyOnContact);
    CHECK_EQ(bullet.GetComponent<LifetimeComponent>().durationMs, 500);
    CHECK_EQ(bullet.GetComponent<TeamComponent>().team, std::string("enemies"));
    CHECK(bullet.GetComponent<CollisionResponseComponent>().onSolid == SolidResponse::Destroy);
}

TEST(TriggerKeyFiresOnlyOnItsKey)
{
    CombatWorld world;
    world.Shooter(glm::vec2(0.0f, 0.0f), ProjectileEmitterComponent(glm::vec2(100.0f, 0.0f), 0, SDLK_SPACE));
    world.registry.Update();
    world.emitter->Update(50000);
    world.eventBus.Emit<KeyPressedEvent>(SDLK_a);
    world.registry.Update();
    CHECK(world.Projectiles().empty());
    world.eventBus.Emit<KeyPressedEvent>(SDLK_SPACE);
    world.registry.Update();
    CHECK_EQ(world.Projectiles().size(), 1u);
}

TEST(EmitterCanAimAlongFacingAtItsSpeed)
{
    CombatWorld world;
    Entity shooter = world.Shooter(glm::vec2(0.0f, 0.0f),
                                   ProjectileEmitterComponent(glm::vec2(100.0f, 0.0f), 0, SDLK_SPACE, true));
    shooter.AddComponent<KeyboardControlledComponent>(50.0f, glm::vec2(0.0f, 1.0f));
    world.registry.Update();
    world.eventBus.Emit<KeyPressedEvent>(SDLK_SPACE);
    world.registry.Update();
    REQUIRE(world.Projectiles().size() == 1u);
    const glm::vec2 velocity = world.Projectiles()[0].GetComponent<RigidBodyComponent>().velocity;
    CHECK_EQ(velocity.x, 0.0f);
    CHECK_EQ(velocity.y, 100.0f);
}

TEST(LifetimeCountsFromWhenTheEntityIsFirstSeen)
{
    CombatWorld world;
    Entity entity = world.registry.CreateEntity();
    entity.AddComponent<LifetimeComponent>(1000);
    world.registry.Update();
    world.lifetime->Update(5000);
    world.lifetime->Update(5999);
    CHECK(entity.IsAlive());
    world.lifetime->Update(6000);
    CHECK(!entity.IsAlive());
}

TEST(DamageAppliesBetweenDifferentTeamsOnly)
{
    CombatWorld world;
    Entity enemy = world.Target(glm::vec2(0.0f, 0.0f), 100, "enemies");
    Entity friendlyFire = world.Damager(glm::vec2(2.0f, 2.0f), 25, true, "enemies");
    Entity player = world.Target(glm::vec2(100.0f, 0.0f), 100, "player");
    Entity hostile = world.Damager(glm::vec2(102.0f, 2.0f), 25, true, "enemies");
    world.Collide();
    CHECK_EQ(enemy.GetComponent<HealthComponent>().health, 100);
    CHECK(friendlyFire.IsAlive());
    CHECK_EQ(player.GetComponent<HealthComponent>().health, 75);
    CHECK(!hostile.IsAlive());
}

TEST(EntitiesWithoutATeamAreOnTheSameUnnamedTeam)
{
    CombatWorld world;
    Entity neutral = world.Target(glm::vec2(0.0f, 0.0f), 100);
    world.Damager(glm::vec2(2.0f, 2.0f), 25, true);
    Entity teamed = world.Target(glm::vec2(100.0f, 0.0f), 100, "player");
    world.Damager(glm::vec2(102.0f, 2.0f), 25, true);
    world.Collide();
    CHECK_EQ(neutral.GetComponent<HealthComponent>().health, 100);
    CHECK_EQ(teamed.GetComponent<HealthComponent>().health, 75);
}

TEST(TargetDiesAtZeroHealth)
{
    CombatWorld world;
    Entity enemy = world.Target(glm::vec2(0.0f, 0.0f), 20, "enemies");
    world.Damager(glm::vec2(2.0f, 2.0f), 20, true, "player");
    world.Collide();
    CHECK(!enemy.IsAlive());
}

TEST(ContactDamageWithoutDestroyKeepsHurtingEachFrame)
{
    CombatWorld world;
    Entity target = world.Target(glm::vec2(0.0f, 0.0f), 100, "player");
    Entity spikes = world.Damager(glm::vec2(2.0f, 2.0f), 10, false, "hazards");
    world.Collide();
    world.Collide();
    CHECK_EQ(target.GetComponent<HealthComponent>().health, 80);
    CHECK(spikes.IsAlive());
}

TEST(TwoDamagingEntitiesWithHealthHurtEachOther)
{
    CombatWorld world;
    Entity a = world.Target(glm::vec2(0.0f, 0.0f), 100, "red");
    a.AddComponent<DamageOnContactComponent>(10);
    Entity b = world.Target(glm::vec2(2.0f, 2.0f), 100, "blue");
    b.AddComponent<DamageOnContactComponent>(30);
    world.Collide();
    CHECK_EQ(a.GetComponent<HealthComponent>().health, 70);
    CHECK_EQ(b.GetComponent<HealthComponent>().health, 90);
}

TEST(SpawnedProjectilesAreDestroyedBySolids)
{
    CombatWorld world;
    world.Shooter(glm::vec2(0.0f, 0.0f), ProjectileEmitterComponent(glm::vec2(0.0f, 0.0f), 0, SDLK_SPACE));
    Entity wall = world.registry.CreateEntity();
    wall.AddComponent<TransformComponent>(glm::vec2(0.0f, 0.0f));
    wall.AddComponent<BoxColliderComponent>(64, 64);
    wall.AddComponent<SolidComponent>();
    world.registry.Update();
    world.eventBus.Emit<KeyPressedEvent>(SDLK_SPACE);
    world.registry.Update();
    REQUIRE(world.Projectiles().size() == 1u);
    Entity bullet = world.Projectiles()[0];
    world.Collide();
    CHECK(!bullet.IsAlive());
    CHECK(wall.IsAlive());
}

TEST(OneProjectileOverlappingTwoTargetsOnlyHitsOne)
{
    CombatWorld world;
    Entity first = world.Target(glm::vec2(0.0f, 0.0f), 100, "enemies");
    Entity second = world.Target(glm::vec2(3.0f, 3.0f), 100, "enemies");
    world.Damager(glm::vec2(4.0f, 4.0f), 10, true, "player");
    world.Collide();
    const int total = first.GetComponent<HealthComponent>().health + second.GetComponent<HealthComponent>().health;
    CHECK_EQ(total, 190);
}
