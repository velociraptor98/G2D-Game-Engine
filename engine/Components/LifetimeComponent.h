#ifndef LIFETIMECOMPONENT_H
#define LIFETIMECOMPONENT_H
#include <SDL2/SDL.h>
// The entity is killed durationMs after the LifetimeSystem first sees it.
struct LifetimeComponent
{
    int durationMs;
    Uint32 startTime;
    bool hasStarted;
    LifetimeComponent(int durationMs = 0) : durationMs(durationMs), startTime(0), hasStarted(false) {}
};
#endif
