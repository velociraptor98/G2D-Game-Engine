#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H
#include <SDL2/SDL.h>
#include <algorithm>
#include <vector>
#include "../ECS/ECS.h"
#include "../AssetManager.h"
#include "../Components/TransformComponent.h"
#include "../Components/SpriteComponent.h"
class RenderSystem : public System
{
public:
    RenderSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<SpriteComponent>();
    }
    void Render(SDL_Renderer *renderer, const AssetManager &assetManager, const SDL_Rect &camera)
    {
        struct Drawable
        {
            const SpriteComponent *sprite;
            const TransformComponent *transform;
            SDL_Rect dstRect;
        };
        std::vector<Drawable> visible;
        visible.reserve(GetEntities().size());
        for (auto entity : GetEntities())
        {
            const auto &transform = entity.GetComponent<TransformComponent>();
            const auto &sprite = entity.GetComponent<SpriteComponent>();
            SDL_Rect dstRect = {
                static_cast<int>(transform.position.x) - (sprite.isFixed ? 0 : camera.x),
                static_cast<int>(transform.position.y) - (sprite.isFixed ? 0 : camera.y),
                static_cast<int>(sprite.width * transform.scale.x),
                static_cast<int>(sprite.height * transform.scale.y)};
            const bool onScreen = dstRect.x + dstRect.w > 0 && dstRect.x < camera.w &&
                                  dstRect.y + dstRect.h > 0 && dstRect.y < camera.h;
            if (onScreen)
            {
                visible.push_back({&sprite, &transform, dstRect});
            }
        }
        std::stable_sort(visible.begin(), visible.end(), [](const Drawable &a, const Drawable &b) {
            return a.sprite->zIndex < b.sprite->zIndex;
        });
        for (const auto &drawable : visible)
        {
            SDL_RenderCopyEx(renderer, assetManager.GetTexture(drawable.sprite->assetId), &drawable.sprite->srcRect,
                             &drawable.dstRect, drawable.transform->rotation, nullptr, drawable.sprite->flip);
        }
    }
};
#endif
