#ifndef ANIMATIONSYSTEM_H
#define ANIMATIONSYSTEM_H
#include <SDL2/SDL.h>
#include <algorithm>
#include "../ECS/ECS.h"
#include "../Components/AnimationComponent.h"
#include "../Components/SpriteComponent.h"
class AnimationSystem : public System
{
public:
    AnimationSystem()
    {
        RequireComponent<SpriteComponent>();
        RequireComponent<AnimationComponent>();
    }
    void Update(Uint32 ticks)
    {
        for (auto entity : GetEntities())
        {
            auto &animation = entity.GetComponent<AnimationComponent>();
            auto &sprite = entity.GetComponent<SpriteComponent>();
            if (animation.numFrames <= 0)
            {
                continue;
            }
            const Uint32 elapsed = ticks > animation.startTime ? ticks - animation.startTime : 0;
            const int frame = static_cast<int>(elapsed * animation.framesPerSecond / 1000);
            animation.currentFrame = animation.isLoop ? frame % animation.numFrames
                                                      : std::min(frame, animation.numFrames - 1);
            sprite.srcRect.x = animation.currentFrame * sprite.width;
        }
    }
};
#endif
