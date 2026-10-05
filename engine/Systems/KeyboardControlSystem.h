#ifndef KEYBOARDCONTROLSYSTEM_H
#define KEYBOARDCONTROLSYSTEM_H
#include <SDL2/SDL.h>
#include "../../lib/glm/glm.hpp"
#include "../ECS/ECS.h"
#include "../Components/KeyboardControlledComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"

class KeyboardControlSystem : public System
{
public:
    KeyboardControlSystem()
    {
        RequireComponent<KeyboardControlledComponent>();
        RequireComponent<RigidBodyComponent>();
    }
    // keyState is indexed by SDL_Scancode, as returned by SDL_GetKeyboardState.
    void Update(const Uint8 *keyState)
    {
        for (auto entity : GetEntities())
        {
            auto &control = entity.GetComponent<KeyboardControlledComponent>();
            auto &rigidBody = entity.GetComponent<RigidBodyComponent>();
            const auto held = [&](Direction direction) {
                for (SDL_Scancode key : control.keys[static_cast<int>(direction)])
                {
                    if (keyState[key])
                        return true;
                }
                return false;
            };
            glm::vec2 direction(0.0f, 0.0f);
            if (held(Direction::Up))
                direction.y -= 1.0f;
            if (held(Direction::Down))
                direction.y += 1.0f;
            if (held(Direction::Left))
                direction.x -= 1.0f;
            if (held(Direction::Right))
                direction.x += 1.0f;
            if (direction.x == 0.0f && direction.y == 0.0f)
            {
                rigidBody.velocity = glm::vec2(0.0f, 0.0f);
                continue;
            }
            control.facing = glm::normalize(direction);
            rigidBody.velocity = control.facing * control.speed;

            const Direction shown = direction.x > 0.0f   ? Direction::Right
                                    : direction.x < 0.0f ? Direction::Left
                                    : direction.y < 0.0f ? Direction::Up
                                                         : Direction::Down;
            const int row = control.spriteRows[static_cast<int>(shown)];
            if (row != KeyboardControlledComponent::NO_SPRITE_ROW && entity.HasComponent<SpriteComponent>())
            {
                auto &sprite = entity.GetComponent<SpriteComponent>();
                sprite.srcRect.y = row * sprite.height;
            }
        }
    }
};
#endif
