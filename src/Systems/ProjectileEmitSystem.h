#ifndef PROJECTILEEMITSYSTEM_H
#define PROJECTILEEMITSYSTEM_H
#include <SDL2/SDL.h>
#include "../../lib/glm/glm.hpp"
#include "../ECS/ECS.h"
#include "../EventBus/EventBus.h"
#include "../Events/KeyPressedEvent.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/ProjectileEmitterComponent.h"
#include "../Components/ProjectileComponent.h"
#include "../Components/KeyboardControlledComponent.h"

const int PROJECTILE_SIZE = 4;

class ProjectileEmitSystem : public System
{
public:
    explicit ProjectileEmitSystem(Registry &registry) : registry(registry)
    {
        RequireComponent<TransformComponent>();
        RequireComponent<ProjectileEmitterComponent>();
    }

    void SubscribeToEvents(EventBus &eventBus)
    {
        eventBus.Subscribe<KeyPressedEvent>([this](KeyPressedEvent &event) {
            if (event.symbol == SDLK_SPACE)
            {
                FirePlayerProjectiles();
            }
        });
    }

    void Update(Uint32 ticks)
    {
        currentTicks = ticks;
        for (auto entity : GetEntities())
        {
            auto &emitter = entity.GetComponent<ProjectileEmitterComponent>();
            if (emitter.repeatFrequency <= 0)
            {
                continue;
            }
            if (ticks - emitter.lastEmissionTime >= static_cast<Uint32>(emitter.repeatFrequency))
            {
                emitter.lastEmissionTime = ticks;
                Spawn(entity, emitter.projectileVelocity);
            }
        }
    }

    void FirePlayerProjectiles()
    {
        for (auto entity : GetEntities())
        {
            if (!entity.HasComponent<KeyboardControlledComponent>())
            {
                continue;
            }
            const glm::vec2 facing = entity.GetComponent<KeyboardControlledComponent>().facing;
            const float speed = glm::length(entity.GetComponent<ProjectileEmitterComponent>().projectileVelocity);
            Spawn(entity, facing * speed);
        }
    }

private:
    // Copies everything it needs from the source first: creating the projectile
    // can grow the component pools and invalidate references into them.
    void Spawn(Entity source, glm::vec2 velocity)
    {
        const TransformComponent transform = source.GetComponent<TransformComponent>();
        const ProjectileEmitterComponent emitter = source.GetComponent<ProjectileEmitterComponent>();
        glm::vec2 centre = transform.position;
        if (source.HasComponent<SpriteComponent>())
        {
            const auto &sprite = source.GetComponent<SpriteComponent>();
            centre += glm::vec2(sprite.width, sprite.height) * transform.scale * 0.5f;
        }
        const glm::vec2 position = centre - glm::vec2(PROJECTILE_SIZE, PROJECTILE_SIZE) * transform.scale * 0.5f;

        Entity projectile = registry.CreateEntity();
        projectile.AddComponent<TransformComponent>(position, transform.scale);
        projectile.AddComponent<RigidBodyComponent>(velocity);
        projectile.AddComponent<SpriteComponent>(emitter.assetId, PROJECTILE_SIZE, PROJECTILE_SIZE, 4);
        projectile.AddComponent<BoxColliderComponent>(PROJECTILE_SIZE, PROJECTILE_SIZE);
        projectile.AddComponent<ProjectileComponent>(emitter.isFriendly, emitter.hitPercentDamage,
                                                     emitter.projectileDuration, currentTicks);
        projectile.Group("projectiles");
    }

    Registry &registry;
    Uint32 currentTicks = 0;
};
#endif
