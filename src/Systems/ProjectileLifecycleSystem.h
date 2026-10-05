#ifndef PROJECTILELIFECYCLESYSTEM_H
#define PROJECTILELIFECYCLESYSTEM_H
#include <SDL2/SDL.h>
#include "../ECS/ECS.h"
#include "../Components/ProjectileComponent.h"
class ProjectileLifecycleSystem : public System
{
public:
    ProjectileLifecycleSystem() { RequireComponent<ProjectileComponent>(); }
    void Update(Uint32 ticks)
    {
        for (auto entity : GetEntities())
        {
            const auto &projectile = entity.GetComponent<ProjectileComponent>();
            if (ticks - projectile.startTime > static_cast<Uint32>(projectile.duration))
            {
                entity.Kill();
            }
        }
    }
};
#endif
