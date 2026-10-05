#ifndef CAMERAMOVEMENTSYSTEM_H
#define CAMERAMOVEMENTSYSTEM_H
#include <SDL2/SDL.h>
#include <algorithm>
#include "../ECS/ECS.h"
#include "../Components/CameraFollowComponent.h"
#include "../Components/TransformComponent.h"
class CameraMovementSystem : public System
{
public:
    CameraMovementSystem()
    {
        RequireComponent<CameraFollowComponent>();
        RequireComponent<TransformComponent>();
    }
    void Update(SDL_Rect &camera, int mapWidth, int mapHeight)
    {
        for (auto entity : GetEntities())
        {
            const auto &transform = entity.GetComponent<TransformComponent>();
            camera.x = static_cast<int>(transform.position.x) - camera.w / 2;
            camera.y = static_cast<int>(transform.position.y) - camera.h / 2;
            camera.x = std::clamp(camera.x, 0, std::max(0, mapWidth - camera.w));
            camera.y = std::clamp(camera.y, 0, std::max(0, mapHeight - camera.h));
        }
    }
};
#endif
