#ifndef RENDERHEALTHBARSYSTEM_H
#define RENDERHEALTHBARSYSTEM_H
#include <SDL2/SDL.h>
#include <algorithm>
#include "../ECS/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/HealthComponent.h"

inline SDL_Color HealthBarColor(int healthPercentage)
{
    if (healthPercentage > 70)
        return SDL_Color{0, 200, 0, 255};
    if (healthPercentage > 40)
        return SDL_Color{230, 200, 0, 255};
    return SDL_Color{220, 0, 0, 255};
}

class RenderHealthBarSystem : public System
{
public:
    static const int BAR_HEIGHT = 4;
    static const int BAR_GAP = 2;

    RenderHealthBarSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<SpriteComponent>();
        RequireComponent<HealthComponent>();
    }
    // Drawn just below the sprite, as wide as the sprite at full health.
    void Render(SDL_Renderer *renderer, const SDL_Rect &camera)
    {
        for (auto entity : GetEntities())
        {
            const auto &transform = entity.GetComponent<TransformComponent>();
            const auto &sprite = entity.GetComponent<SpriteComponent>();
            const int health = std::clamp(entity.GetComponent<HealthComponent>().healthPercentage, 0, 100);
            const int fullWidth = static_cast<int>(sprite.width * transform.scale.x);
            const SDL_Rect bar = {
                static_cast<int>(transform.position.x) - camera.x,
                static_cast<int>(transform.position.y + sprite.height * transform.scale.y) + BAR_GAP - camera.y,
                fullWidth * health / 100, BAR_HEIGHT};
            const SDL_Color color = HealthBarColor(health);
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(renderer, &bar);
        }
    }
};
#endif
