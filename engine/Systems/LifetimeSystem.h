#ifndef LIFETIMESYSTEM_H
#define LIFETIMESYSTEM_H
#include <SDL2/SDL.h>
#include "../ECS/ECS.h"
#include "../Components/LifetimeComponent.h"
class LifetimeSystem : public System
{
public:
    LifetimeSystem() { RequireComponent<LifetimeComponent>(); }
    void Update(Uint32 ticks)
    {
        for (auto entity : GetEntities())
        {
            auto &lifetime = entity.GetComponent<LifetimeComponent>();
            if (!lifetime.hasStarted)
            {
                lifetime.hasStarted = true;
                lifetime.startTime = ticks;
            }
            if (ticks - lifetime.startTime >= static_cast<Uint32>(lifetime.durationMs))
            {
                entity.Kill();
            }
        }
    }
};
#endif
