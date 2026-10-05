#ifndef DAMAGESYSTEM_H
#define DAMAGESYSTEM_H
#include "../ECS/ECS.h"
#include "../EventBus/EventBus.h"
#include "../Events/CollisionEvent.h"
#include "../Components/HealthComponent.h"
#include "../Components/ProjectileComponent.h"

// Friendly projectiles hurt "enemies", the others hurt the "player", and
// "obstacles" absorb both. Projectiles pass through everything else.
class DamageSystem : public System
{
public:
    DamageSystem() { RequireComponent<HealthComponent>(); }

    void SubscribeToEvents(EventBus &eventBus)
    {
        eventBus.Subscribe<CollisionEvent>([this](CollisionEvent &event) {
            if (!HandleHit(event.a, event.b))
            {
                HandleHit(event.b, event.a);
            }
        });
    }

private:
    bool HandleHit(Entity projectile, Entity target)
    {
        if (!projectile.BelongsToGroup("projectiles") || !projectile.IsAlive() || !target.IsAlive())
        {
            return false;
        }
        if (target.BelongsToGroup("obstacles"))
        {
            projectile.Kill();
            return true;
        }
        const auto &hit = projectile.GetComponent<ProjectileComponent>();
        const bool isTarget = hit.isFriendly ? target.BelongsToGroup("enemies") : target.HasTag("player");
        if (!isTarget || !target.HasComponent<HealthComponent>())
        {
            return false;
        }
        auto &health = target.GetComponent<HealthComponent>();
        health.healthPercentage -= hit.hitPercentDamage;
        projectile.Kill();
        if (health.healthPercentage <= 0)
        {
            target.Kill();
        }
        return true;
    }
};
#endif
