#ifndef COLLISIONRESPONSESYSTEM_H
#define COLLISIONRESPONSESYSTEM_H
#include <SDL2/SDL.h>
#include <algorithm>
#include "../ECS/ECS.h"
#include "../EventBus/EventBus.h"
#include "../Events/CollisionEvent.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/SolidComponent.h"
#include "../Components/CollisionResponseComponent.h"
#include "./CollisionSystem.h"

class CollisionResponseSystem : public System
{
public:
    CollisionResponseSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<CollisionResponseComponent>();
    }

    void SubscribeToEvents(EventBus &eventBus)
    {
        eventBus.Subscribe<CollisionEvent>([this](CollisionEvent &event) {
            Respond(event.a, event.b);
            Respond(event.b, event.a);
        });
    }

private:
    void Respond(Entity mover, Entity other)
    {
        if (!mover.IsAlive() || !mover.HasComponent<CollisionResponseComponent>() ||
            !other.HasComponent<SolidComponent>())
        {
            return;
        }
        const auto &response = mover.GetComponent<CollisionResponseComponent>();
        switch (response.onSolid)
        {
        case SolidResponse::None:
            break;
        case SolidResponse::Destroy:
            mover.Kill();
            break;
        case SolidResponse::Bounce:
            Bounce(mover, other, response.flipSpriteOnBounce);
            break;
        case SolidResponse::Block:
            Block(mover, other);
            break;
        }
    }

    // Only reverses while still heading into the solid; reversing on every
    // overlapping frame would make the entity jitter in place.
    static void Bounce(Entity mover, Entity solid, bool flipSprite)
    {
        if (!mover.HasComponent<RigidBodyComponent>())
        {
            return;
        }
        auto &rigidBody = mover.GetComponent<RigidBodyComponent>();
        const glm::vec2 towardsSolid = Centre(solid) - Centre(mover);
        bool reversedX = false;
        if (rigidBody.velocity.x * towardsSolid.x > 0.0f)
        {
            rigidBody.velocity.x = -rigidBody.velocity.x;
            reversedX = true;
        }
        if (rigidBody.velocity.y * towardsSolid.y > 0.0f)
        {
            rigidBody.velocity.y = -rigidBody.velocity.y;
        }
        if (reversedX && flipSprite && mover.HasComponent<SpriteComponent>())
        {
            auto &sprite = mover.GetComponent<SpriteComponent>();
            sprite.flip = sprite.flip == SDL_FLIP_NONE ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        }
    }

    // Pushes the mover out along the axis of least overlap and stops its
    // velocity on that axis, so it slides along walls instead of sticking.
    static void Block(Entity mover, Entity solid)
    {
        auto &transform = mover.GetComponent<TransformComponent>();
        const SDL_FRect a = ColliderBounds(transform, mover.GetComponent<BoxColliderComponent>());
        const SDL_FRect b =
            ColliderBounds(solid.GetComponent<TransformComponent>(), solid.GetComponent<BoxColliderComponent>());
        const float overlapX = std::min(a.x + a.w, b.x + b.w) - std::max(a.x, b.x);
        const float overlapY = std::min(a.y + a.h, b.y + b.h) - std::max(a.y, b.y);
        const bool pushAlongX = overlapX < overlapY;
        const float direction = pushAlongX ? (a.x + a.w / 2 < b.x + b.w / 2 ? -1.0f : 1.0f)
                                           : (a.y + a.h / 2 < b.y + b.h / 2 ? -1.0f : 1.0f);
        if (pushAlongX)
        {
            transform.position.x += direction * overlapX;
        }
        else
        {
            transform.position.y += direction * overlapY;
        }
        if (mover.HasComponent<RigidBodyComponent>())
        {
            auto &velocity = mover.GetComponent<RigidBodyComponent>().velocity;
            float &along = pushAlongX ? velocity.x : velocity.y;
            if (along * direction < 0.0f)
            {
                along = 0.0f;
            }
        }
    }

    static glm::vec2 Centre(Entity entity)
    {
        const SDL_FRect bounds =
            ColliderBounds(entity.GetComponent<TransformComponent>(), entity.GetComponent<BoxColliderComponent>());
        return glm::vec2(bounds.x + bounds.w / 2, bounds.y + bounds.h / 2);
    }
};
#endif
