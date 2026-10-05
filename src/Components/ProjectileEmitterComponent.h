#ifndef PROJECTILEEMITTERCOMPONENT_H
#define PROJECTILEEMITTERCOMPONENT_H
#include <SDL2/SDL.h>
#include <string>
#include <utility>
#include "../../lib/glm/glm.hpp"
// A repeatFrequency of 0 means the emitter only fires on demand (the player's
// fire key); the player fires along its facing at the speed of projectileVelocity.
struct ProjectileEmitterComponent
{
    glm::vec2 projectileVelocity;
    int repeatFrequency;
    int projectileDuration;
    int hitPercentDamage;
    bool isFriendly;
    std::string assetId;
    Uint32 lastEmissionTime;
    ProjectileEmitterComponent(glm::vec2 projectileVelocity = glm::vec2(0.0f, 0.0f), int repeatFrequency = 0,
                               int projectileDuration = 10000, int hitPercentDamage = 10, bool isFriendly = false,
                               std::string assetId = "")
        : projectileVelocity(projectileVelocity), repeatFrequency(repeatFrequency),
          projectileDuration(projectileDuration), hitPercentDamage(hitPercentDamage), isFriendly(isFriendly),
          assetId(std::move(assetId)), lastEmissionTime(0)
    {
    }
};
#endif
