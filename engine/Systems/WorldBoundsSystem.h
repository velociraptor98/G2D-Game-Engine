#ifndef WORLDBOUNDSSYSTEM_H
#define WORLDBOUNDSSYSTEM_H
#include <algorithm>
#include "../ECS/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/StayInBoundsComponent.h"
class WorldBoundsSystem : public System
{
public:
    WorldBoundsSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<StayInBoundsComponent>();
    }
    void Update(int worldWidth, int worldHeight)
    {
        for (auto entity : GetEntities())
        {
            auto &transform = entity.GetComponent<TransformComponent>();
            float width = 0.0f;
            float height = 0.0f;
            if (entity.HasComponent<SpriteComponent>())
            {
                const auto &sprite = entity.GetComponent<SpriteComponent>();
                width = sprite.width * transform.scale.x;
                height = sprite.height * transform.scale.y;
            }
            transform.position.x = std::clamp(transform.position.x, 0.0f, std::max(0.0f, worldWidth - width));
            transform.position.y = std::clamp(transform.position.y, 0.0f, std::max(0.0f, worldHeight - height));
        }
    }
};
#endif
