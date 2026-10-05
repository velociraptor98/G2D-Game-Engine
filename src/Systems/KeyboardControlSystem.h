#ifndef KEYBOARDCONTROLSYSTEM_H
#define KEYBOARDCONTROLSYSTEM_H
#include <SDL2/SDL.h>
#include "../../lib/glm/glm.hpp"
#include "../ECS/ECS.h"
#include "../Components/KeyboardControlledComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"

// Directional spritesheets have one row per facing, in this order.
enum class SpriteRow
{
    Down = 0,
    Right = 1,
    Left = 2,
    Up = 3
};

class KeyboardControlSystem : public System
{
public:
    KeyboardControlSystem()
    {
        RequireComponent<KeyboardControlledComponent>();
        RequireComponent<RigidBodyComponent>();
        RequireComponent<SpriteComponent>();
    }
    // keyState is indexed by SDL_Scancode, as returned by SDL_GetKeyboardState.
    void Update(const Uint8 *keyState)
    {
        glm::vec2 direction(0.0f, 0.0f);
        if (keyState[SDL_SCANCODE_UP] || keyState[SDL_SCANCODE_W])
            direction.y -= 1.0f;
        if (keyState[SDL_SCANCODE_DOWN] || keyState[SDL_SCANCODE_S])
            direction.y += 1.0f;
        if (keyState[SDL_SCANCODE_LEFT] || keyState[SDL_SCANCODE_A])
            direction.x -= 1.0f;
        if (keyState[SDL_SCANCODE_RIGHT] || keyState[SDL_SCANCODE_D])
            direction.x += 1.0f;
        const bool moving = direction.x != 0.0f || direction.y != 0.0f;

        for (auto entity : GetEntities())
        {
            auto &control = entity.GetComponent<KeyboardControlledComponent>();
            auto &rigidBody = entity.GetComponent<RigidBodyComponent>();
            auto &sprite = entity.GetComponent<SpriteComponent>();
            if (!moving)
            {
                rigidBody.velocity = glm::vec2(0.0f, 0.0f);
                continue;
            }
            control.facing = glm::normalize(direction);
            rigidBody.velocity = control.facing * control.speed;
            SpriteRow row = direction.x > 0.0f   ? SpriteRow::Right
                            : direction.x < 0.0f ? SpriteRow::Left
                            : direction.y < 0.0f ? SpriteRow::Up
                                                 : SpriteRow::Down;
            sprite.srcRect.y = static_cast<int>(row) * sprite.height;
        }
    }
};
#endif
