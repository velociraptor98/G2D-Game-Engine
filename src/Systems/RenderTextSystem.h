#ifndef RENDERTEXTSYSTEM_H
#define RENDERTEXTSYSTEM_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../ECS/ECS.h"
#include "../AssetManager.h"
#include "../Components/TextLabelComponent.h"
class RenderTextSystem : public System
{
public:
    RenderTextSystem() { RequireComponent<TextLabelComponent>(); }
    // Rasterises every label each frame; fine for a handful of HUD strings.
    void Render(SDL_Renderer *renderer, const AssetManager &assetManager, const SDL_Rect &camera)
    {
        for (auto entity : GetEntities())
        {
            const auto &label = entity.GetComponent<TextLabelComponent>();
            TTF_Font *font = assetManager.GetFont(label.fontId);
            if (!font || label.text.empty())
            {
                continue;
            }
            SDL_Surface *surface = TTF_RenderUTF8_Blended(font, label.text.c_str(), label.color);
            if (!surface)
            {
                continue;
            }
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect dstRect = {static_cast<int>(label.position.x), static_cast<int>(label.position.y), surface->w,
                                surface->h};
            SDL_FreeSurface(surface);
            if (label.isCentred)
            {
                dstRect.x -= dstRect.w / 2;
                dstRect.y -= dstRect.h / 2;
            }
            if (!label.isFixed)
            {
                dstRect.x -= camera.x;
                dstRect.y -= camera.y;
            }
            SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
            SDL_DestroyTexture(texture);
        }
    }
};
#endif
