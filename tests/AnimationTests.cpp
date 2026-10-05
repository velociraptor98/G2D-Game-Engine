#include "./TestFramework.h"
#include "ECS/ECS.h"
#include "Components/SpriteComponent.h"
#include "Components/AnimationComponent.h"
#include "Systems/AnimationSystem.h"

namespace
{
    struct AnimatedSprite
    {
        Registry registry;
        AnimationSystem *system = nullptr;
        Entity entity = registry.CreateEntity();

        AnimatedSprite(int numFrames, int framesPerSecond, bool isLoop, Uint32 startTime = 0)
        {
            system = &registry.AddSystem<AnimationSystem>();
            entity.AddComponent<SpriteComponent>("sheet", 32, 32, 0, false, 0, 64);
            entity.AddComponent<AnimationComponent>(numFrames, framesPerSecond, isLoop, startTime);
            registry.Update();
        }
        const SDL_Rect &SrcRect() { return entity.GetComponent<SpriteComponent>().srcRect; }
    };
}

TEST(AnimationAdvancesFramesAtTheGivenRate)
{
    AnimatedSprite sprite(4, 10, true);
    sprite.system->Update(0);
    CHECK_EQ(sprite.SrcRect().x, 0);
    sprite.system->Update(99);
    CHECK_EQ(sprite.SrcRect().x, 0);
    sprite.system->Update(100);
    CHECK_EQ(sprite.SrcRect().x, 32);
    sprite.system->Update(350);
    CHECK_EQ(sprite.SrcRect().x, 96);
}

TEST(LoopingAnimationWrapsToTheFirstFrame)
{
    AnimatedSprite sprite(4, 10, true);
    sprite.system->Update(400);
    CHECK_EQ(sprite.entity.GetComponent<AnimationComponent>().currentFrame, 0);
    sprite.system->Update(500);
    CHECK_EQ(sprite.SrcRect().x, 32);
}

TEST(NonLoopingAnimationHoldsTheLastFrame)
{
    AnimatedSprite sprite(4, 10, false);
    sprite.system->Update(10000);
    CHECK_EQ(sprite.SrcRect().x, 96);
}

TEST(AnimationIsTimedFromItsStartTime)
{
    AnimatedSprite sprite(4, 10, false, 1000);
    sprite.system->Update(500);
    CHECK_EQ(sprite.SrcRect().x, 0);
    sprite.system->Update(1100);
    CHECK_EQ(sprite.SrcRect().x, 32);
}

TEST(AnimationLeavesTheSpritesheetRowAlone)
{
    AnimatedSprite sprite(2, 10, true);
    sprite.system->Update(100);
    CHECK_EQ(sprite.SrcRect().y, 64);
}
