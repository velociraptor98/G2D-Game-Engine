#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H
#include <SDL2/SDL.h>
#include <string>
#include <utility>
struct SpriteComponent
{
    std::string assetId;
    int width;
    int height;
    int zIndex;
    // Fixed sprites are drawn in screen space and ignore the camera (HUD elements).
    bool isFixed;
    SDL_Rect srcRect;
    SDL_RendererFlip flip;
    SpriteComponent(std::string assetId = "", int width = 0, int height = 0, int zIndex = 0, bool isFixed = false,
                    int srcRectX = 0, int srcRectY = 0)
        : assetId(std::move(assetId)), width(width), height(height), zIndex(zIndex), isFixed(isFixed),
          srcRect{srcRectX, srcRectY, width, height}, flip(SDL_FLIP_NONE)
    {
    }
};
#endif
