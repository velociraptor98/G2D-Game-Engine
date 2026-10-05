#ifndef PROJECTILEEMITTERCOMPONENT_H
#define PROJECTILEEMITTERCOMPONENT_H
#include <SDL2/SDL.h>
#include <string>
#include <utility>
#include "../../lib/glm/glm.hpp"

// What each spawned projectile looks like and does. Projectiles join their
// shooter's team, damage what they touch once, and die on solids.
struct ProjectileTemplate
{
    std::string assetId;
    int width = 4;
    int height = 4;
    int zIndex = 4;
    int damage = 10;
    int lifetimeMs = 3000;
};

// Fires on a timer (repeatFrequency > 0), on a key press (triggerKey), or both.
struct ProjectileEmitterComponent
{
    glm::vec2 projectileVelocity;
    int repeatFrequency;
    SDL_Keycode triggerKey;
    // When set, fires along the entity's KeyboardControlledComponent facing at
    // the speed of projectileVelocity instead of in its fixed direction.
    bool aimAlongFacing;
    ProjectileTemplate projectile;
    Uint32 lastEmissionTime;

    ProjectileEmitterComponent(glm::vec2 projectileVelocity = glm::vec2(0.0f, 0.0f), int repeatFrequency = 0,
                               SDL_Keycode triggerKey = SDLK_UNKNOWN, bool aimAlongFacing = false,
                               ProjectileTemplate projectile = ProjectileTemplate())
        : projectileVelocity(projectileVelocity), repeatFrequency(repeatFrequency), triggerKey(triggerKey),
          aimAlongFacing(aimAlongFacing), projectile(std::move(projectile)), lastEmissionTime(0)
    {
    }
};
#endif
