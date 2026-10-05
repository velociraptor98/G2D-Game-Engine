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
#include "../Components/KeyboardControlledComponent.h"
#include "../Components/LifetimeComponent.h"
#include "../Components/DamageOnContactComponent.h"
#include "../Components/TeamComponent.h"
#include "../Components/CollisionResponseComponent.h"

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
        eventBus.Subscribe<KeyPressedEvent>([this](KeyPressedEvent &event) { OnKeyPressed(event.symbol); });
    }

    void Update(Uint32 ticks)
    {
        for (auto entity : GetEntities())
        {
            auto &emitter = entity.GetComponent<ProjectileEmitterComponent>();
            if (emitter.repeatFrequency > 0 &&
                ticks - emitter.lastEmissionTime >= static_cast<Uint32>(emitter.repeatFrequency))
            {
                emitter.lastEmissionTime = ticks;
                Fire(entity);
            }
        }
    }

    void OnKeyPressed(SDL_Keycode key)
    {
        for (auto entity : GetEntities())
        {
            if (key != SDLK_UNKNOWN && entity.GetComponent<ProjectileEmitterComponent>().triggerKey == key)
            {
                Fire(entity);
            }
        }
    }

private:
    // Copies everything it needs from the source first: creating the projectile
    // can grow the component pools and invalidate references into them.
    void Fire(Entity source)
    {
        const TransformComponent transform = source.GetComponent<TransformComponent>();
        const ProjectileEmitterComponent emitter = source.GetComponent<ProjectileEmitterComponent>();
        glm::vec2 velocity = emitter.projectileVelocity;
        if (emitter.aimAlongFacing && source.HasComponent<KeyboardControlledComponent>())
        {
            velocity = source.GetComponent<KeyboardControlledComponent>().facing * glm::length(velocity);
        }
        glm::vec2 centre = transform.position;
        if (source.HasComponent<SpriteComponent>())
        {
            const auto &sprite = source.GetComponent<SpriteComponent>();
            centre += glm::vec2(sprite.width, sprite.height) * transform.scale * 0.5f;
        }
        const ProjectileTemplate &shot = emitter.projectile;
        const glm::vec2 position = centre - glm::vec2(shot.width, shot.height) * transform.scale * 0.5f;
        const bool hasTeam = source.HasComponent<TeamComponent>();
        const TeamComponent team = hasTeam ? source.GetComponent<TeamComponent>() : TeamComponent();

        Entity projectile = registry.CreateEntity();
        projectile.AddComponent<TransformComponent>(position, transform.scale);
        projectile.AddComponent<RigidBodyComponent>(velocity);
        projectile.AddComponent<SpriteComponent>(shot.assetId, shot.width, shot.height, shot.zIndex);
        projectile.AddComponent<BoxColliderComponent>(shot.width, shot.height);
        projectile.AddComponent<LifetimeComponent>(shot.lifetimeMs);
        projectile.AddComponent<DamageOnContactComponent>(shot.damage, true);
        projectile.AddComponent<CollisionResponseComponent>(SolidResponse::Destroy);
        if (hasTeam)
        {
            projectile.AddComponent<TeamComponent>(team);
        }
    }

    Registry &registry;
};
#endif
