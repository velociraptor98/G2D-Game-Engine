#ifndef DAMAGESYSTEM_H
#define DAMAGESYSTEM_H
#include <string>
#include "../ECS/ECS.h"
#include "../EventBus/EventBus.h"
#include "../Events/CollisionEvent.h"
#include "../Components/HealthComponent.h"
#include "../Components/TeamComponent.h"
#include "../Components/DamageOnContactComponent.h"

inline std::string TeamOf(Entity entity)
{
    return entity.HasComponent<TeamComponent>() ? entity.GetComponent<TeamComponent>().team : "";
}

// An entity with DamageOnContact hurts anything with Health on a different
// team that it collides with; entities at 0 health are killed.
class DamageSystem : public System
{
public:
    DamageSystem() { RequireComponent<HealthComponent>(); }

    void SubscribeToEvents(EventBus &eventBus)
    {
        eventBus.Subscribe<CollisionEvent>([](CollisionEvent &event) {
            ApplyDamage(event.a, event.b);
            ApplyDamage(event.b, event.a);
        });
    }

private:
    static void ApplyDamage(Entity attacker, Entity target)
    {
        if (!attacker.IsAlive() || !target.IsAlive() || !attacker.HasComponent<DamageOnContactComponent>() ||
            !target.HasComponent<HealthComponent>() || TeamOf(attacker) == TeamOf(target))
        {
            return;
        }
        const auto &damage = attacker.GetComponent<DamageOnContactComponent>();
        auto &health = target.GetComponent<HealthComponent>();
        health.health -= damage.damage;
        if (damage.destroyOnContact)
        {
            attacker.Kill();
        }
        if (health.health <= 0)
        {
            target.Kill();
        }
    }
};
#endif
