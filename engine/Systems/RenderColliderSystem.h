#ifndef RENDERCOLLIDERSYSTEM_H
#define RENDERCOLLIDERSYSTEM_H
#include <SDL2/SDL.h>
#include "../ECS/ECS.h"
#include "./CollisionSystem.h"
class RenderColliderSystem : public System
{
public:
    RenderColliderSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<BoxColliderComponent>();
    }
    void Render(SDL_Renderer *renderer, const SDL_Rect &camera)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        for (auto entity : GetEntities())
        {
            const SDL_FRect bounds = ColliderBounds(entity.GetComponent<TransformComponent>(),
                                                    entity.GetComponent<BoxColliderComponent>());
            const SDL_Rect outline = {static_cast<int>(bounds.x) - camera.x, static_cast<int>(bounds.y) - camera.y,
                                      static_cast<int>(bounds.w), static_cast<int>(bounds.h)};
            SDL_RenderDrawRect(renderer, &outline);
        }
    }
};
#endif
