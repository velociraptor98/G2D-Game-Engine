#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H
#include <SDL2/SDL.h>
#include "../ECS/ECS.h"
#include "../EventBus/EventBus.h"
#include "../Events/CollisionEvent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/TransformComponent.h"

inline SDL_FRect ColliderBounds(const TransformComponent &transform, const BoxColliderComponent &collider)
{
    return SDL_FRect{transform.position.x + collider.offset.x * transform.scale.x,
                     transform.position.y + collider.offset.y * transform.scale.y,
                     collider.width * transform.scale.x, collider.height * transform.scale.y};
}

// Boxes that only touch along an edge do not collide, so neighbouring tiles stay quiet.
inline bool Overlaps(const SDL_FRect &a, const SDL_FRect &b)
{
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

class CollisionSystem : public System
{
public:
    CollisionSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<BoxColliderComponent>();
    }
    void Update(EventBus &eventBus)
    {
        const auto &entities = GetEntities();
        for (std::size_t i = 0; i < entities.size(); ++i)
        {
            const Entity a = entities[i];
            if (!a.IsAlive())
            {
                continue;
            }
            const SDL_FRect aBounds =
                ColliderBounds(a.GetComponent<TransformComponent>(), a.GetComponent<BoxColliderComponent>());
            for (std::size_t j = i + 1; j < entities.size(); ++j)
            {
                const Entity b = entities[j];
                if (!b.IsAlive())
                {
                    continue;
                }
                const SDL_FRect bBounds =
                    ColliderBounds(b.GetComponent<TransformComponent>(), b.GetComponent<BoxColliderComponent>());
                if (Overlaps(aBounds, bBounds))
                {
                    eventBus.Emit<CollisionEvent>(a, b);
                    if (!a.IsAlive())
                    {
                        break;
                    }
                }
            }
        }
    }
};
#endif
