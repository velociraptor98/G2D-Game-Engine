#include "./TestFramework.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "ECS/ECS.h"
#include "Components/TransformComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/KeyboardControlledComponent.h"
#include "Systems/KeyboardControlSystem.h"
#include "Systems/MovementSystem.h"
#include "Systems/WorldBoundsSystem.h"
#include "Components/StayInBoundsComponent.h"

namespace
{
    struct ControlledPlayer
    {
        Registry registry;
        KeyboardControlSystem *system = &registry.AddSystem<KeyboardControlSystem>();
        Entity entity = registry.CreateEntity();
        std::vector<Uint8> keys = std::vector<Uint8>(SDL_NUM_SCANCODES, 0);

        ControlledPlayer()
        {
            entity.AddComponent<RigidBodyComponent>();
            entity.AddComponent<SpriteComponent>("chopper", 32, 32);
            auto &control = entity.AddComponent<KeyboardControlledComponent>(100.0f);
            control.spriteRows = {3, 0, 2, 1};
            registry.Update();
        }
        void Press(std::initializer_list<SDL_Scancode> pressed)
        {
            std::fill(keys.begin(), keys.end(), 0);
            for (auto key : pressed)
            {
                keys[key] = 1;
            }
            system->Update(keys.data());
        }
        glm::vec2 Velocity() { return entity.GetComponent<RigidBodyComponent>().velocity; }
        int Row() { return entity.GetComponent<SpriteComponent>().srcRect.y / 32; }
    };
}

TEST(ArrowKeysSetVelocityAndSpritesheetRow)
{
    ControlledPlayer player;
    player.Press({SDL_SCANCODE_RIGHT});
    CHECK_EQ(player.Velocity().x, 100.0f);
    CHECK_EQ(player.Velocity().y, 0.0f);
    CHECK_EQ(player.Row(), 1);
    player.Press({SDL_SCANCODE_LEFT});
    CHECK_EQ(player.Velocity().x, -100.0f);
    CHECK_EQ(player.Row(), 2);
    player.Press({SDL_SCANCODE_UP});
    CHECK_EQ(player.Velocity().y, -100.0f);
    CHECK_EQ(player.Row(), 3);
    player.Press({SDL_SCANCODE_DOWN});
    CHECK_EQ(player.Velocity().y, 100.0f);
    CHECK_EQ(player.Row(), 0);
}

TEST(WasdWorksLikeArrowKeys)
{
    ControlledPlayer player;
    player.Press({SDL_SCANCODE_D});
    CHECK_EQ(player.Velocity().x, 100.0f);
    player.Press({SDL_SCANCODE_W});
    CHECK_EQ(player.Velocity().y, -100.0f);
}

TEST(DiagonalMovementIsNotFaster)
{
    ControlledPlayer player;
    player.Press({SDL_SCANCODE_RIGHT, SDL_SCANCODE_DOWN});
    CHECK(std::abs(glm::length(player.Velocity()) - 100.0f) < 0.01f);
    CHECK_EQ(player.Row(), 1);
}

TEST(ReleasingKeysStopsButKeepsFacing)
{
    ControlledPlayer player;
    player.Press({SDL_SCANCODE_LEFT});
    player.Press({});
    CHECK_EQ(player.Velocity().x, 0.0f);
    CHECK_EQ(player.Velocity().y, 0.0f);
    CHECK_EQ(player.Row(), 2);
    CHECK_EQ(player.entity.GetComponent<KeyboardControlledComponent>().facing.x, -1.0f);
}

TEST(OpposingKeysCancelOut)
{
    ControlledPlayer player;
    player.Press({SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT});
    CHECK_EQ(player.Velocity().x, 0.0f);
}

TEST(SpriteRowIsLeftAloneWhenNoRowsAreConfigured)
{
    ControlledPlayer player;
    player.entity.GetComponent<KeyboardControlledComponent>().spriteRows.fill(KeyboardControlledComponent::NO_SPRITE_ROW);
    player.entity.GetComponent<SpriteComponent>().srcRect.y = 64;
    player.Press({SDL_SCANCODE_LEFT});
    CHECK_EQ(player.Row(), 2);
    CHECK_EQ(player.Velocity().x, -100.0f);
}

TEST(CustomKeyBindingsReplaceTheDefaults)
{
    ControlledPlayer player;
    auto &keys = player.entity.GetComponent<KeyboardControlledComponent>().keys;
    keys[static_cast<int>(Direction::Right)] = {SDL_SCANCODE_L};
    player.Press({SDL_SCANCODE_RIGHT});
    CHECK_EQ(player.Velocity().x, 0.0f);
    player.Press({SDL_SCANCODE_L});
    CHECK_EQ(player.Velocity().x, 100.0f);
}

TEST(KeyboardControlWorksWithoutASprite)
{
    Registry registry;
    auto &system = registry.AddSystem<KeyboardControlSystem>();
    Entity entity = registry.CreateEntity();
    entity.AddComponent<RigidBodyComponent>();
    entity.AddComponent<KeyboardControlledComponent>(10.0f);
    registry.Update();
    std::vector<Uint8> keys(SDL_NUM_SCANCODES, 0);
    keys[SDL_SCANCODE_DOWN] = 1;
    system.Update(keys.data());
    CHECK_EQ(entity.GetComponent<RigidBodyComponent>().velocity.y, 10.0f);
}

TEST(StayInBoundsKeepsTheSpriteInsideTheWorldButOthersCanLeave)
{
    Registry registry;
    auto &movement = registry.AddSystem<MovementSystem>();
    auto &bounds = registry.AddSystem<WorldBoundsSystem>();
    Entity bounded = registry.CreateEntity();
    bounded.AddComponent<TransformComponent>(glm::vec2(90.0f, 5.0f), glm::vec2(2.0f, 2.0f));
    bounded.AddComponent<RigidBodyComponent>(glm::vec2(100.0f, -100.0f));
    bounded.AddComponent<SpriteComponent>("chopper", 8, 8);
    bounded.AddComponent<StayInBoundsComponent>();
    Entity free = registry.CreateEntity();
    free.AddComponent<TransformComponent>(glm::vec2(90.0f, 5.0f));
    free.AddComponent<RigidBodyComponent>(glm::vec2(100.0f, -100.0f));
    registry.Update();

    movement.Update(1.0f);
    bounds.Update(100, 100);
    CHECK_EQ(bounded.GetComponent<TransformComponent>().position.x, 84.0f);
    CHECK_EQ(bounded.GetComponent<TransformComponent>().position.y, 0.0f);
    CHECK_EQ(free.GetComponent<TransformComponent>().position.x, 190.0f);
    CHECK_EQ(free.GetComponent<TransformComponent>().position.y, -95.0f);
}
