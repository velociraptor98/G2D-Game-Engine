#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H
#include "../Game.h"
#include <SDL2/SDL.h>
#include "../TextureManager.h"
#include "../AssetManager.h"
#include "./TransformComponent.h"
#include "../Animation.h"
#include <map>
#include <string>
class SpriteComponent : public Component {
    private:
    TransformComponent* transform = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_Rect sourceRectangle{0,0,0,0};
    SDL_Rect destinationRectangle{0,0,0,0};
    bool isAnimated = false;
    int numFrame = 0;
    int animationSpeed = 0;
    bool isFixed = false;
    std::map<std::string,Animation> animations;
    std::string currentAnimationName;
    unsigned int animationIndex = 0;
    public:
    SDL_RendererFlip spriteFlip = SDL_FLIP_NONE;
    SpriteComponent(std::string assetTextureId){
        setTexture(assetTextureId);
    }
    void setTexture(std::string assetTextureId){
        texture = Game::assetManager->getTexture(assetTextureId);
    }
    void init() override{
        transform = owner->getComponent<TransformComponent>();
        sourceRectangle.x = 0 ;
        sourceRectangle.y = 0;
        sourceRectangle.w = transform->width;
        sourceRectangle.h = transform->height;
    }
    void Update(float deltaTime) override{
        destinationRectangle.x = (int)transform->position.x;
        destinationRectangle.y = (int)transform->position.y;
        destinationRectangle.w = transform->width * transform->scale;
        destinationRectangle.h = transform->height * transform->scale;
    }
    void Render() override{
        TextureManager::Draw(texture,sourceRectangle,destinationRectangle,spriteFlip);
    }

};
#endif