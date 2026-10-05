#ifndef ANIMATIONCOMPONENT_H
#define ANIMATIONCOMPONENT_H
#include <SDL2/SDL.h>
// Frames are laid out left to right on the sprite's current spritesheet row.
struct AnimationComponent
{
    int numFrames;
    int framesPerSecond;
    bool isLoop;
    Uint32 startTime;
    int currentFrame;
    AnimationComponent(int numFrames = 1, int framesPerSecond = 1, bool isLoop = true, Uint32 startTime = 0)
        : numFrames(numFrames), framesPerSecond(framesPerSecond), isLoop(isLoop), startTime(startTime), currentFrame(0)
    {
    }
};
#endif
