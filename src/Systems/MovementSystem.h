#ifndef MOVEMENTSYSTEM_H
#define MOVEMENTSYSTEM_H
#include <algorithm>
#include "../ECS/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../EventBus/EventBus.h"
#include "../Events/CollisionEvent.h"
class MovementSystem : public System
{
public:
    MovementSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<RigidBodyComponent>();
    }
    void SubscribeToEvents(EventBus &eventBus)
    {
        eventBus.Subscribe<CollisionEvent>([this](CollisionEvent &event) { OnCollision(event); });
    }

    // The entity tagged "player" is kept fully inside the map.
    void Update(float deltaTime, int mapWidth, int mapHeight)
    {
        for (auto entity : GetEntities())
        {
            auto &transform = entity.GetComponent<TransformComponent>();
            const auto &rigidBody = entity.GetComponent<RigidBodyComponent>();
            transform.position += rigidBody.velocity * deltaTime;
            if (entity.HasTag("player"))
            {
                float width = 0.0f;
                float height = 0.0f;
                if (entity.HasComponent<SpriteComponent>())
                {
                    const auto &sprite = entity.GetComponent<SpriteComponent>();
                    width = sprite.width * transform.scale.x;
                    height = sprite.height * transform.scale.y;
                }
                transform.position.x = std::clamp(transform.position.x, 0.0f, std::max(0.0f, mapWidth - width));
                transform.position.y = std::clamp(transform.position.y, 0.0f, std::max(0.0f, mapHeight - height));
            }
        }
    }

private:
    void OnCollision(CollisionEvent &event)
    {
        if (event.a.BelongsToGroup("enemies") && event.b.BelongsToGroup("obstacles"))
        {
            BounceOff(event.a, event.b);
        }
        else if (event.b.BelongsToGroup("enemies") && event.a.BelongsToGroup("obstacles"))
        {
            BounceOff(event.b, event.a);
        }
    }

    // Only reverses while still heading into the obstacle; reversing on every
    // overlapping frame would make the enemy jitter in place.
    void BounceOff(Entity enemy, Entity obstacle)
    {
        if (!enemy.HasComponent<RigidBodyComponent>())
        {
            return;
        }
        auto &rigidBody = enemy.GetComponent<RigidBodyComponent>();
        const glm::vec2 towardsObstacle = Centre(obstacle) - Centre(enemy);
        bool reversed = false;
        if (rigidBody.velocity.x * towardsObstacle.x > 0.0f)
        {
            rigidBody.velocity.x = -rigidBody.velocity.x;
            reversed = true;
        }
        if (rigidBody.velocity.y * towardsObstacle.y > 0.0f)
        {
            rigidBody.velocity.y = -rigidBody.velocity.y;
            reversed = true;
        }
        if (reversed && rigidBody.velocity.x != 0.0f && enemy.HasComponent<SpriteComponent>())
        {
            auto &sprite = enemy.GetComponent<SpriteComponent>();
            sprite.flip = sprite.flip == SDL_FLIP_NONE ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        }
    }

    static glm::vec2 Centre(Entity entity)
    {
        const auto &transform = entity.GetComponent<TransformComponent>();
        const auto &collider = entity.GetComponent<BoxColliderComponent>();
        return transform.position +
               (collider.offset + glm::vec2(collider.width, collider.height) * 0.5f) * transform.scale;
    }
};
#endif
