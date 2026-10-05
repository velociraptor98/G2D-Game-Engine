#ifndef PROJECTILECOMPONENT_H
#define PROJECTILECOMPONENT_H
#include <SDL2/SDL.h>
struct ProjectileComponent
{
    bool isFriendly;
    int hitPercentDamage;
    int duration;
    Uint32 startTime;
    ProjectileComponent(bool isFriendly = false, int hitPercentDamage = 0, int duration = 0, Uint32 startTime = 0)
        : isFriendly(isFriendly), hitPercentDamage(hitPercentDamage), duration(duration), startTime(startTime)
    {
    }
};
#endif
