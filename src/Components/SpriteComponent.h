#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H
#include "../Game.h"
#include <SDL2/SDL.h>
#include "../TextureManager.h"
#include "../AssetManager.h"
#include "./TransformComponent.h"
class SpriteComponent : public Component {
    private:
    TransformComponent* transform;
    SDL_Texture* texture;
    SDL_Rect sourceRectangle;
    SDL_Rect destinationRectangle;
    bool isAnimated;
    int numFrame;
    int animationSpeed;
    bool isFixed;
    std::map<std::string,Animation> animations;
    std::string currentAnimationName;
    unsigned int animationIndex;
    public:
    SDL_RendererFlip spriteFlip = SDL_FLIP_NONE;
    SpriteComponent(const char* filePath){
        setTexture(filePath);
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